module.exports = [
  {
    "type": "heading",
    "defaultValue": "IgiWatch Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background"
      },
      {
        "type": "color",
        "messageKey": "TextColor",
        "defaultValue": "0xFFFFFF",
        "label": "Text"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Font"
      },
      {
        "type": "radiogroup",
        "messageKey": "FontChoice",
        "label": "Choose Font",
        "defaultValue": "0",
        "options": [
          { "label": "Avenir Next", "value": "0" },
          { "label": "Leco 1976", "value": "1" },
          { "label": "Avenir Next Regular", "value": "2" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Orientation"
      },
      {
        "type": "radiogroup",
        "messageKey": "ScreenOrientation",
        "label": "Choose Orientation",
        "defaultValue": "0",
        "options": [
          { "label": "Automatic (Double Tap)", "value": "0" },
          { "label": "Always Portrait", "value": "1" },
          { "label": "Always Landscape (Left)", "value": "2" },
          { "label": "Always Landscape (Right)", "value": "3" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Date Language"
      },
      {
        "type": "radiogroup",
        "messageKey": "DateLanguage",
        "label": "Choose Language (Portrait Mode Only)",
        "defaultValue": "it_IT",
        "options": [
          { "label": "Italian", "value": "it_IT" },
          { "label": "English", "value": "en_US" },
          { "label": "French", "value": "fr_FR" },
          { "label": "German", "value": "de_DE" },
          { "label": "Spanish", "value": "es_ES" }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
