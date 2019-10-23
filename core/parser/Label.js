const Node = require('./Node');
const Utils = require('./Utils');
const state = require('./Global').state;

class Label extends Node {
    constructor(data) {
        super(data);
        this._label_text = '';
        this._jsonNode.object_type = 'Label';
    }

    parse_properties() {
        super.parse_properties();

        // Move Node properties into 'node' and clean _properties
        this._properties = {node: this._properties};

        let component = Node.get_node_component_of_type(this._node_data, 'cc.Label');

        let is_system_font = component._isSystemFontUsed;
        this._properties.fontSize = component._fontSize;
        this._properties.labelText = component._N$string;

        // outline
        let outline_component = Node.get_node_component_of_type(this._node_data, 'cc.LabelOutline');
        if (outline_component) {
            let color = outline_component._color;
            let outline_info = {
                color: {
                    r: color.r,
                    g: color.g,
                    b: color.b,
                    a: color.a
                },
                width: outline_component._width
            }
            this._properties.outline = outline_info;
        }

        /*
        cc.LabelShadow structure
        {
            "__type__": "cc.LabelShadow",
            "_name": "",
            "_objFlags": 0,
            "node": {
            "__id__": 91
            },
            "_enabled": true,
            "_color": {
            "__type__": "cc.Color",
            "r": 0,
            "g": 0,
            "b": 0,
            "a": 255
            },
            "_offset": {
            "__type__": "cc.Vec2",
            "x": 2,
            "y": -2
            },
            "_blur": 2,
            "_id": "77NYefe6hHnY6NnvyTl0cL"
        },
        */

        // Shadow
        let shadow_component = Node.get_node_component_of_type(this._node_data, 'cc.LabelShadow');
        if (shadow_component) {
            let color = shadow_component._color;
            let offset = shadow_component._offset;
            let shadow_info = {
                color: {
                    r: color.r,
                    g: color.g,
                    b: color.b,
                    a: color.a
                },
                offset: {
                    x: offset.x,
                    y: offset.y
                },
                blurRadius: shadow_component._blur
            }

            this._properties.shadow = shadow_info;
        }

        // alignments
        this._properties.horizontalAlignment = Label.H_ALIGNMENTS[component._N$horizontalAlign];
        this._properties.verticalAlignment = Label.V_ALIGNMENTS[component._N$verticalAlign];

        this._properties.overflowType = Label.OVERFLOW_TYPE[component._N$overflow];
        this.add_property_bool('enableWrap', '_enableWrapText', component);

        if (is_system_font) {
            this._properties.fontType = 'System';
            this._properties.fontName = 'arial';
        }
        else {
            let fontName = Utils.get_font_path_by_uuid(component._N$file.__uuid__);
            this._properties.fontName = state._assetpath + fontName;
            if (fontName.endsWith('.ttf'))
                this._properties.fontType = 'TTF';
            else if (fontName.endsWith('.fnt')) {
                this._properties.fontType = 'BMFont';
                this.add_property_int('fontSize', '_fontSize', component);
            }
            else
                throw 'can not find font file for uuid: ' + component._N$file.__uuid__;

            this.add_property_int('lineHeight' ,'_lineHeight', component);
        }
    }
}
Label.H_ALIGNMENTS = ['Left', 'Center', 'Right'];
Label.V_ALIGNMENTS = ['Top', 'Center', 'Bottom'];
Label.OVERFLOW_TYPE = ['None', 'Clamp', 'Shrink', 'ResizeHeight'];

module.exports = Label;