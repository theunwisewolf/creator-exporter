const Node = require('./Node');
const state = require('./Global').state;
const Utils = require('./Utils');

class EditBox extends Node {
    constructor(data) {
        super(data);
        this._jsonNode.object_type = 'EditBox';
    }

    parse_properties() {
        super.parse_properties();

        this._properties = {node: this._properties};

        let component = Node.get_node_component_of_type(this._node_data, 'cc.EditBox');

        // background image is needed by cocos2d-x's EditBox
        if (!component._N$backgroundImage || component._N$backgroundImage == null)
        {
            Utils.log("Error: EditBox background image is needed by cocos2d-x!");
            this._properties.backgroundImage = "";
        }
        else
        {
            this._properties.backgroundImage = Utils.get_sprite_frame_name_by_uuid(component._N$backgroundImage.__uuid__);
        }

        // Find the TEXT_LABEL child
        if (this._node_data._children)
        {
            this._node_data._children.forEach(function(item) {
                let node = state._json_data[item.__id__];
                if (node._name == "TEXT_LABEL") {
                    let component = Node.get_node_component_of_type(node, 'cc.Label');

                    this._properties.horizontalAlignment = EditBox.H_ALIGNMENTS[component._N$horizontalAlign];
                    this._properties.verticalAlignment = EditBox.V_ALIGNMENTS[component._N$verticalAlign];
                    this.add_property_int('fontSize', '_fontSize', component);
                    this.add_property_rgb('fontColor', '_color', node);
                } else if (node._name == "PLACEHOLDER_LABEL") {
                    let component = Node.get_node_component_of_type(node, 'cc.Label');

                    this.add_property_str('placeholder', '_string', component);
                    this.add_property_int('placeholderFontSize', '_fontSize', component);
                    this.add_property_rgb('placeholderFontColor', '_color', node);
                }
            }.bind(this));
        }


        this._properties.returnType = EditBox.RETURN_TYPE[component.returnType];
        this._properties.inputFlag = EditBox.INPUT_FLAG[component._N$inputFlag];
        this._properties.inputMode = EditBox.INPUT_MODE[component._N$inputMode];
        
        this.add_property_int('maxLength', 'maxLength', component);
        this.add_property_str('text', '_string', component);
    }
}
EditBox.H_ALIGNMENTS = ['Left', 'Center', 'Right'];
EditBox.V_ALIGNMENTS = ['Top', 'Center', 'Bottom'];
EditBox.OVERFLOW_TYPE = ['None', 'Clamp', 'Shrink', 'ResizeHeight'];
EditBox.INPUT_MODE = ['Any', 'EmailAddress', 'Numeric', 'PhoneNumber', 'URL', 'Decime', 'SingleLine'];
EditBox.INPUT_FLAG = ['Password', 'Sensitive', 'InitialCapsWord', 'InitialCapsSentence', 'InitialCapsAllCharacters', 'LowercaseAllCharacters'];
EditBox.RETURN_TYPE = ['Default', 'Done', 'Send', 'Search', 'Go'];

module.exports = EditBox;