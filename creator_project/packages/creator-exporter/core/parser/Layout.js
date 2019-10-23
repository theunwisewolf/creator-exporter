const Node = require('./Node');
const Utils = require('./Utils');
const state = require('./Global').state;

class Layout extends Node {
    // Example Layout structure
    /* {
        "__type__": "cc.Layout",
        "_name": "",
        "_objFlags": 0,
        "node": {
            "__id__": 4
        },
        "_enabled": true,
        "_layoutSize": {
            "__type__": "cc.Size",
            "width": 200,
            "height": 150
        },
        "_resize": 0,
        "_N$layoutType": 0,
        "_N$padding": 0,
        "_N$cellSize": {
            "__type__": "cc.Size",
            "width": 40,
            "height": 40
        },
        "_N$startAxis": 0,
        "_N$paddingLeft": 0,
        "_N$paddingRight": 0,
        "_N$paddingTop": 0,
        "_N$paddingBottom": 0,
        "_N$spacingX": 0,
        "_N$spacingY": 0,
        "_N$verticalDirection": 1,
        "_N$horizontalDirection": 0
    } */
    constructor(data) {
        super(data);
        this._jsonNode.object_type = 'Layout';
    }

    parse_properties() {
        super.parse_properties();
        this._properties = {
			node: this._properties
		};

        // let spr_component = Node.get_node_component_of_type(this._node_data, 'cc.Sprite');
        // this._properties.backgroundVisible = spr_component._enabled;

		let component = Node.get_node_component_of_type(this._node_data, 'cc.Layout');

		// Grab the data from the layout component
		// Global properties
        this._properties.layoutType = component._N$layoutType;
		this._properties.resizeMode = component._resize;
		this._properties.spacingX = component._N$spacingX;
		this._properties.spacingY = component._N$spacingY;
		this._properties.affectedByScale = component._N$affectedByScale;

		// Grid properties
		this._properties.axisDirection = component._N$startAxis;
		this._properties.paddingLeft = component._N$paddingLeft;
		this._properties.paddingRight = component._N$paddingRight;
		this._properties.paddingTop = component._N$paddingTop;
		this._properties.paddingBottom = component._N$paddingBottom;
		this._properties.cellSize = { w: component._N$cellSize.width, h: component._N$cellSize.height };

		// Vertical layout properties
		this._properties.verticalDirection = component._N$verticalDirection;

		// Horizontal layout properties
		this._properties.horizontalDirection = component._N$horizontalDirection;
    }
}

module.exports = Layout;