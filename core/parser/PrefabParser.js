const fs = require('fs');
const fire_fs = require('fire-fs');
const path = require('path');
const Utils = require('../Utils');
const Constants = require('../Constants');
const Scene = require('./Scene');
const state = require('./Global').state;
const get_sprite_frame_name_by_uuid = require('./Utils').get_sprite_frame_name_by_uuid;

let uuidInfos = null;

/**
 * bootstrap + helper functions
 */
class PrefabParser {
    constructor() {
        this._state = state;
        this._json_file = null;
        this._json_output = { version: Constants.VERDION, root: {} };
        this._creatorassets = null;
    }

    to_json_setup() {
        this.to_json_setup_design_resolution();
        this.to_json_setup_sprite_frames();
    }

    to_json_setup_design_resolution() {
        // Access the editor settings and get the design resolution
        let settings = Editor.remote.Profile.load('profile://project/project.json').data;
        this._json_output.designResolution = {
            w: settings['design-resolution-width'],
            h: settings['design-resolution-height']
        };
    }

    to_json_setup_sprite_frames() {
        let sprite_frames = [];

        for (let sprite_frame_uuid in state._sprite_frames) {
            let sprite_frame = state._sprite_frames[sprite_frame_uuid];
            let frameName = get_sprite_frame_name_by_uuid(sprite_frame_uuid).replace(/(split\_qualities\/)|(no_split\/)/, "");

            // Spriteframe not used in prefab
            // if (state._usedSpriteFrames.indexOf(frameName) == -1)
            // {
            //     continue;
            // }

            // Do not parse texture packer atlas
            // NOTE: Handled internally (This is done to support multiple resolutions)
            // I am doing this here because each prefab will otherwise contain unnecesarry information about
            // spriteframes. Most (All) of the spriteframes are already loaded by Scenes
            // If in case, an atlas is present that is only used by prefabs and not by any scene, the game
            // will crash, because it's information will not be present
            if (sprite_frame.is_texture_packer)
                continue;

            let frame = {
                name: frameName,
                texturePath: state._assetpath + sprite_frame.texture_path,
                rect: { x: sprite_frame.trimX, y: sprite_frame.trimY, w: sprite_frame.width, h: sprite_frame.height },
                offset: { x: sprite_frame.offsetX, y: sprite_frame.offsetY },
                rotated: sprite_frame.rotated,
                originalSize: { w: sprite_frame.rawWidth, h: sprite_frame.rawHeight }
            };
            // does it have a capInsets?
            if (sprite_frame.borderTop != 0 || sprite_frame.borderBottom != 0 ||
                sprite_frame.borderLeft != 0 || sprite_frame.borderRgith != 0) {

                frame.centerRect = {
                    x: sprite_frame.borderLeft,
                    y: sprite_frame.borderTop,
                    w: sprite_frame.width - sprite_frame.borderRight - sprite_frame.borderLeft,
                    h: sprite_frame.height - sprite_frame.borderBottom - sprite_frame.borderTop
                }
            }

            frame.atlas = sprite_frame.is_texture_packer

            sprite_frames.push(frame);
        }

        this._json_output.spriteFrames = sprite_frames;
    }

    create_file(filename) {
        fire_fs.ensureDirSync(path.dirname(filename));
        return fs.openSync(filename, 'w');
    }

    run(filename, assetpath, path_to_json_files) {
        state._filename = path.basename(filename, '.prefab');
        let sub_folder = path.dirname(filename).substr(Constants.ASSETS_PATH.length + 1);
        let json_name = path.join(path_to_json_files, sub_folder, state._filename) + '.jsonp';
        this._json_file = this.create_file(json_name);
        state._assetpath = assetpath;

        state._json_data = JSON.parse(fs.readFileSync(filename));

        state._json_data.forEach(obj => {
            if (obj.__type__ === 'cc.Prefab') {
                let prefab = obj.data;
                let prefab_idx = prefab.__id__;
                //let prefab_obj = new Scene(state._json_data[prefab_idx]);

                let prefab_obj = new Scene({
                    "_children": [
                        {
                            "__id__": prefab_idx
                        }
                    ],
                    "_opacity": 255,
                    "_color": {
                        "__type__": "cc.Color",
                        "r": 255,
                        "g": 255,
                        "b": 255,
                        "a": 255
                    },
                    "_contentSize": {
                        "__type__": "cc.Size",
                        "width": state._json_data[prefab_idx]._contentSize.width,
                        "height": state._json_data[prefab_idx]._contentSize.height
                    },
                    "_anchorPoint": {
                        "__type__": "cc.Vec2",
                        "x": 0.5,
                        "y": 0.5
                    },
                    "_position": {
                        "__type__": "cc.Vec3",
                        "x": 0,
                        "y": 0,
                        "z": 0
                    },
                    "_scale": {
                        "__type__": "cc.Vec3",
                        "x": 1,
                        "y": 1,
                        "z": 1
                    },
                });

                prefab_obj.parse_properties();

                this.to_json_setup();
                let jsonNode = prefab_obj.to_json(0, 0);

                jsonNode.object_type = "Prefab";

                this._json_output.root = jsonNode;
                let dump = JSON.stringify(this._json_output, null, '\t').replace(/\\\\/g, '/');
                fs.writeSync(this._json_file, dump);
                fs.close(this._json_file);
            }
        });

        // Parse any animations
        Object.keys(state._clips).forEach((key) => {
            let object = state._clips[key];
            Utils.log("Parsing animation clip:" + object.name);
            let filepath = path.join(Constants.JSON_ANIMATIONS_PATH, object.name) + '.animation';
            fire_fs.ensureDirSync(path.dirname(filepath));
            let file = fs.openSync(filepath, 'w');

            let dump = JSON.stringify(object, null, '\t').replace(/\\\\/g, '/');
            fs.writeSync(file, dump);
            fs.close(file);

            // Compile the file to binary
			let params = ['-b', '-o', Constants.ANIMATIONS_BINARY_PATH, Constants.CREATOR_ANIMATION_FBS, filepath];

			Utils.runcommand(Constants.FLATC, params, function (code) {
                if (code != 0)
                {
                    Utils.failed('Error while compiling ' + filepath);
                    return;
                }
			});
        });

        // Empty the clips
        state._clips = {};
    }
}

function parse_prefab(filenames, assetpath, path_to_json_files, uuidmaps) {
    if (assetpath[-1] != '/')
        assetpath += '/';

    uuidinfos = uuidmaps;

    let uuid = {};

    filenames.forEach(function (filename) {
        Utils.info("Parsing prefab file: " + filename);

        state.reset();

        let parser = new PrefabParser();
        parser.run(filename, assetpath, path_to_json_files)
        for (let key in state._uuid) {
            if (state._uuid.hasOwnProperty(key))
                uuid[key] = state._uuid[key];
        }
    });

    return uuid;
}

module.exports = parse_prefab;