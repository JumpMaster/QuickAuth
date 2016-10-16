module.exports =
  [
  { 
    "type": "heading", 
    "defaultValue": "QuickAuth" 
  }, 
  { 
    "type": "text", 
    "defaultValue": "Google Authenticator for Pebble" 
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Key Settings"
      },
      { 
        "type": "text", 
        "defaultValue": "Add or edit an existing key." 
      },
      {
        "type": "input",
        "messageKey": "auth_name",
        "label": "Name",
        "attributes": {
          "placeholder": "eg: Google",
          "limit": 20
        }
      },
      {
        "type": "input",
        "messageKey": "auth_key",
        "label": "Key",
        "attributes": {
          "limit": 128
        }
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "App Settings"
      },
      { 
        "type": "text", 
        "defaultValue": "Settings that can be changed without having to add or edit a key." 
      },
      {
        "type": "color",
        "messageKey": "foreground_color",
        "defaultValue": "ffffff",
        "label": "Foreground Color",
        "sunlight": false,
      },
      {
        "type": "color",
        "messageKey": "background_color",
        "defaultValue": "0000ff",
        "label": "Background Color",
        "sunlight": false,
      },
      {
        "type": "select",
        "messageKey": "idle_timeout",
        "label": "Idle Timeout",
        "defaultValue": "300",
        "options": [
          { 
            "label": "Disabled", 
            "value": "0" 
          },
          { 
            "label": "1 Minute", 
            "value": "60" 
          },
          { 
            "label": "2 Minutes", 
            "value": "120" 
          },
          {
            "label": "5 Minutes",
            "value": "300"
          },
          {
            "label": "10 Minutes",
            "value": "600"
          }
        ]
      },
      {
        "type": "radiogroup",
        "messageKey": "window_layout",
        "label": "View Mode",
        "defaultValue": "0",
        "options": [
          { 
            "label": "Single Code", 
            "value": "0" 
          },
          { 
            "label": "Multi Code", 
            "value": "1" 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "font",
        "label": "Font",
        "defaultValue": "1",
        "options": [
          { 
            "label": "Style 1", 
            "value": "0" 
          },
          { 
            "label": "Style 2", 
            "value": "1" 
          },
          { 
            "label": "Style 3", 
            "value": "2" 
          },
          {
            "label": "Style 4",
            "value": "3"
          }
        ]
      },
      {
        "type": "submit",
        "defaultValue": "Save"
      }
    ]
  }
];