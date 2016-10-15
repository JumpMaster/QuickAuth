module.exports =
[
  { 
    "type": "heading", 
    "defaultValue": "Example Header Item" 
  }, 
  { 
    "type": "text", 
    "defaultValue": "Example text item." 
  },
  {
    "type": "color",
    "messageKey": "foreground_color",
    "defaultValue": "0000ff",
    "label": "Foreground Color",
    "sunlight": false,
  },
  {
    "type": "color",
    "messageKey": "background_color",
    "defaultValue": "ffffff",
    "label": "Background Color",
    "sunlight": false,
  },
  {
    "type": "radiogroup",
    "messageKey": "font",
    "label": "Font",
    "defaultValue": "1",
    "options": [
      { 
        "label": "Font Style 1", 
        "value": "0" 
      },
      { 
        "label": "Font Style 2", 
        "value": "1" 
      },
      { 
        "label": "Font Style 3", 
        "value": "2" 
      },
      {
        "label": "Font Style 4",
        "value": "3"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];