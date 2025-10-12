module.exports = [
  {
    "type": "heading",
    "defaultValue": "Analogue"
  },
  {
    "type": "text",
    "defaultValue": "<p>by astosia</p>"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Features"
      },
      {
        "type": "toggle",
        "messageKey": "InvertScreen",
        "label": "Invert Screen",
        "description": "OFF = Black, ON = White",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "JumpHourOn",
        "label": "Hour Digit Movement",
        "description": "OFF = Smooth (Analogue Sweep), ON = Step (Digital Jump Hour)",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "AddZero12h",
        "label": "Add leading zero to 12h time",
        "description": "Applies when 12h time selected in watch settings",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "RemoveZero24h",
        "label": "Remove leading zero from 24h time",
        "description": "Applies when 24h time selected in watch settings",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "BTVibeOn",
        "label": "Bluetooth Disconnect Vibe",
        "description": "OFF = Never Vibrate, ON = Vibrate (when quiet time is off)",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
]
