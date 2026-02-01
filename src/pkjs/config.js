module.exports = [
  {
    "type": "heading",
    "defaultValue": "Helix"
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
        "messageKey": "showlocalAMPM",
        "label": "Show AM or PM for local time",
        "description": "Applies when 12h time selected in watch settings",
        "defaultValue": true
      },
      // {
      //   "type": "toggle",
      //   "messageKey": "ampm_onminutering",
      //   "label": "Show AM/PM on minute ring",
      //   "description": "OFF = shows on Hour ring/left, ON = shows on Minute ring/right",
      //   "defaultValue": false
      // },
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
      },
      {
        "type": "toggle",
        "messageKey": "TZ_MODE",
        "label": "Show 2nd Timezone",
        "description": "OFF = Don't show, ON = Show",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "showremoteAMPM",
        "label": "Show AM or PM for 2nd timezone",
        "description": "Applies when 12h time selected in watch settings",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "TZ_ID",
        "label": "2nd Timezone",
        "description": "Data provided by worldtimeapi.org",
        "options": [
          { "label": "Select a Zone", "value": "" }
          ]
      },
      {
        "type": "input",
        "messageKey": "TZ_ID_STATE",
        "defaultValue": ""
      },
      {
        "type": "button",
        "id": "TZ_BUTTON",
        "primary": true,
        "defaultValue": "Fetch Timezones",
        "description": "Tap to reload the list.  If this fails on save, 2nd timezone will show as 99"
      },
      {
        "type": "text",
        "id": "TZ_DEBUG",
        "defaultValue": ""
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];