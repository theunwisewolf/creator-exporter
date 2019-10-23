
/* jslint node: true, sub: true, esversion: 6, browser: true */
/* globals Editor */

"use strict";
const Path = require('path');

const Utils = require('./Utils');
const Constants = require('./Constants');
const fs = require('fs');
const path = require('path');
const Fs = require('fire-fs');
const Del = require('del')
const parse_fire = require('./parser/ConvertFireToJson');
const parse_prefab = require('./parser/PrefabParser');
const parse_utils = require('./parser/Utils')

const { WorkerBase, registerWorker } = require('./WorkerBase');

class BuildWorker extends WorkerBase {
	run(state, callback) {
		// Utils.recordBuild();

		Editor.Ipc.sendToAll('creator-exporter:state-changed', 'start', 0);
		Utils.info('Begin compiling...');

		this._callback = callback;
		this._state = state;

		// clean old json or ccreator files
		Fs.emptyDirSync(Constants.JSON_PATH);
		Fs.emptyDirSync(Constants.CCREATOR_PATH);

		Utils.getAssetsInfo(function (uuidmap) {
			let prefabResources = this._getPrefabResources(uuidmap);
			let copyResourceInfos = this._convertFireToJson(uuidmap);
			let dynamicLoadRes = this._getDynamicLoadRes(uuidmap);

			Object.assign(copyResourceInfos, prefabResources);
			Object.assign(copyResourceInfos, dynamicLoadRes);

			this._compileJsonToBinary(function () {
				this._copyResources(copyResourceInfos);
				Editor.Ipc.sendToAll('creator-exporter:state-changed', 'finish', 100);
				this._callback();
				Utils.info('Finished');
			}.bind(this));
		}.bind(this));
	}

	_getPrefabResources(uuidmap) {
		let prefabs = this._getPrefabList();

		// Filter out the unchanged files
		prefabs = this._getChangedFiles(prefabs);

		let resources = parse_prefab(prefabs, 'creator', Constants.JSON_PATH, uuidmap);

		return resources;
	}

	_convertFireToJson(uuidmap) {
		let fireFiles = this._getFireList();

		// Filter out the unchanged files
		fireFiles = this._getChangedFiles(fireFiles);

		let copyResourceInfos = parse_fire(fireFiles, 'creator', Constants.JSON_PATH, uuidmap);

		return copyResourceInfos;
	}

	// .json -> .ccreator
	_compileJsonToBinary(cb) {
		const jsonFiles = this._getJsonList();
		const jsonPrefabFiles = this._getJsonPrefabList();

		// No files were changed
		if (jsonPrefabFiles.length == 0 && jsonFiles.length == 0) {
			Utils.success("A total of 0 files were processed.");
			cb();
			return;
		}

		let i = 0;
		jsonFiles.forEach(function (file) {
			let subFolder = Path.dirname(file).substr(Constants.JSON_PATH.length + 1);
			let creatorPath = Path.join(Constants.CCREATOR_PATH, subFolder);
			let params = ['-b', '-o', creatorPath, Constants.CREATOR_READER_FBS, file];

			Utils.runcommand(Constants.FLATC, params, function (code) {
				if (code != 0)
					Utils.failed('Error while compiling ' + file);

				++i;

				if (i === (jsonFiles.length + jsonPrefabFiles.length)) {
					Utils.info("A total of " + i + " files were processed.");
					cb();
				}
			});
		});

		jsonPrefabFiles.forEach(function (file) {
			let subFolder = Path.dirname(file).substr(Constants.JSON_PATH.length + 1);
			let creatorPath = Path.join(Constants.CCREATOR_PATH, subFolder);
			let params = ['-b', '-o', creatorPath, Constants.CREATOR_READER_FBS, file];

			Utils.runcommand(Constants.FLATC, params, function (code) {
				if (code != 0)
				Utils.failed('Error while compiling ' + file);

				++i;
				
				if (i === (jsonFiles.length + jsonPrefabFiles.length)) {
					Utils.info("A total of " + i + " files were processed.");
					cb();
				}
			});
		});
	}

	_copyResources(copyResourceInfos) {
		// should copy these resources
		// - all .ccreator files
		// - resources in assets and folder
		// - all files in reader
		// - lua binding codes(currently is missing)
		let projectRoot = this._state.path;

		// root path of resources
		let resdst;
		let classes;
		let isLuaProject = Utils.isLuaProject(projectRoot);
		if (isLuaProject) {
			resdst = Path.join(projectRoot, 'res');

			classes = Path.join(projectRoot, 'frameworks/runtime-src/Classes');
			if (!Fs.existsSync(classes)) {
				classes = Path.join(projectRoot, 'project/Classes'); // cocos2d-x internal lua tests
			}
		} else {
			resdst = Path.join(projectRoot, 'Resources');
			classes = Path.join(projectRoot, 'Classes');
		}

		// copy resources
		{
			// copy .ccreator
			resdst = Path.join(resdst, Constants.RESOURCE_FOLDER_NAME);
			// Del.sync(resdst, { force: true });
			this._copyTo(Constants.CCREATOR_PATH, resdst, ['.ccreator'], true);

			// copy other resources
			Object.keys(copyResourceInfos).forEach(function (uuid) {
				let pathInfo = copyResourceInfos[uuid];
				let src = pathInfo.fullpath;
				let dst = Path.join(resdst, pathInfo.relative_path);
				Fs.ensureDirSync(Path.dirname(dst));
				Fs.copySync(src, dst);
			});
		}

		let state = Editor.remote.Profile.load('profile://project/' + Constants.PACKAGE_NAME + '.json', Constants.PROFILE_DEFAULTS);
		if (state.data.exportResourceOnly)
			return;

		// copy reader
		{
			let codeFilesDest = Path.join(classes, 'reader');
			Utils.walkAsync(Constants.READER_PATH, function (err, sourceFiles) {
				let changedFiles = [];
				sourceFiles.forEach((sourceFile) => {
					let stat = fs.statSync(sourceFile);
					let sourcemTime = stat.mtime.getTime();
					let relativePath = sourceFile.replace(Constants.READER_PATH + "/", "");
					let destFilePath = Path.join(codeFilesDest, relativePath);

					if (!fs.existsSync(destFilePath)) {
						Utils.warn(Path.basename(destFilePath) + " does not exist! Copying... " + destFilePath);
						changedFiles.push({ "src": sourceFile, "dst": destFilePath });
					} else {
						stat = fs.statSync(destFilePath);
						let destmTime = stat.mtime.getTime();

						// File has been changed and must be copied
						if (sourcemTime > destmTime) {
							// Delete the file first
							fs.unlinkSync(destFilePath);
							changedFiles.push({ "src": sourceFile, "dst": destFilePath });
						}
					}
				});

				changedFiles.forEach((obj) => {
					Utils.log("Reader file " + Path.basename(obj.src) + " was copied because it was changed.");

					Fs.ensureDirSync(Path.dirname(obj.dst));
					Fs.copySync(obj.src, obj.dst);
				});
			});

			// Del.sync(codeFilesDest, { force: true });
			// Fs.copySync(Constants.READER_PATH, codeFilesDest);

			// should exclude binding codes for c++ project
			if (!isLuaProject) {
				let bindingCodesPath = Path.join(classes, 'reader/lua-bindings');
				Del.sync(bindingCodesPath, { force: true });
			}
		}
	}

	_getChangedFiles(filenames) {
		let changedFiles = [];

		// This file stores the modify time for Reader
		let timeFilePath = Path.join(Constants.TEMP_PATH, "creator_files.time");

		if (fs.existsSync(timeFilePath)) {
			let data = fs.readFileSync(timeFilePath);
			let json = JSON.parse(data);

			filenames.forEach((filename) => {
				let stat = fs.statSync(filename);
				let mtime = stat.mtime.getTime();
			
				if (json[filename] != null) {
					// File was modified, add to changed files
					if (json[filename] < mtime) {
						json[filename] = mtime;
						changedFiles.push(filename);
					}
				} else { // No data present, add to changed files
					json[filename] = mtime;
					changedFiles.push(filename);
				}
			});

			// Write the time file back
			let file = fs.openSync(timeFilePath, 'w');
			let dump = JSON.stringify(json, null, '\t');
			fs.writeSync(file, dump);
			fs.close(file);
		} else {
			Utils.warn("Time file was not found, creating a new one");

			// Mark all files as changed
			let json = {};

			filenames.forEach((filename) => {
				let stat = fs.statSync(filename);
				let mtime = stat.mtime.getTime();

				json[filename] = mtime;
				changedFiles.push(filename);
			});

			// Create the file first if it isn't there
			Fs.ensureDirSync(Path.dirname(timeFilePath));

			// Write the time file back
			let file = fs.openSync(timeFilePath, 'w');
			let dump = JSON.stringify(json, null, '\t');
			fs.writeSync(file, dump);
			fs.close(file);
		}

		return changedFiles;
	}

	// copy all files with ext in src to dst
	// @exts array of ext, such as ['.json', '.ccreator']
	// @recursive whether recursively to copy the subfolder
	_copyTo(src, dst, exts, recursive) {
		let files = this._getFilesWithExt(src, exts, recursive);

		let dstpath;
		let subpath;
		files.forEach((f) => {
			subpath = f.slice(src.length, f.length);
			dstpath = Path.join(dst, subpath);
			Fs.ensureDirSync(Path.dirname(dstpath));
			Fs.copySync(f, dstpath);
		});
	}

	// get all .prefab files
	_getPrefabList() {
		return this._getFilesWithExt(Constants.ASSETS_PATH, ['.prefab'], true);
	}

	// get all .fire file in assets folder
	_getFireList() {
		return this._getFilesWithExt(Constants.ASSETS_PATH, ['.fire'], true);
	}

	_getJsonList() {
		return this._getFilesWithExt(Constants.JSON_PATH, ['.json'], true);
	}

	_getJsonPrefabList() {
		return this._getFilesWithExt(Constants.JSON_PATH, ['.jsonp'], true);
	}

	// return file list ends with `exts` in dir
	_getFilesWithExt(dir, exts, recursive) {
		let foundFiles = [];

		const files = Fs.readdirSync(dir);
		files.forEach((f) => {
			let fullpath = Path.join(dir, f)
			let ext = Path.extname(f);
			if (exts.includes(ext))
				foundFiles.push(fullpath);

			if (recursive) {
				let stats = Fs.lstatSync(fullpath);
				if (stats.isDirectory())
					foundFiles = foundFiles.concat(this._getFilesWithExt(fullpath, exts, recursive));
			}
		});
		return foundFiles;
	}

	// dynamically load resources located at assets/resources folder
	_getDynamicLoadRes(uuidmap, collectedResources) {
		let state = Editor.remote.Profile.load('profile://project/' + Constants.PACKAGE_NAME + '.json', Constants.PROFILE_DEFAULTS);
		if (!state.data.exportResourceDynamicallyLoaded)
			return;

		let dynamicLoadRes = {};
		let resourcesPath = Path.join(Constants.ASSETS_PATH, 'resources');

		Object.keys(uuidmap).forEach(function (uuid) {
			if (uuidmap[uuid].indexOf(resourcesPath) < 0)
				return true;

			dynamicLoadRes[uuid] = parse_utils.get_relative_full_path_by_uuid(uuid);
		});

		return dynamicLoadRes;
	}
}

registerWorker(BuildWorker, 'run-build-worker');
