module.exports = [
  {
    "type": "heading",
    "defaultValue": "Configurazione IgiWatch"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colori"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Sfondo"
      },
      {
        "type": "color",
        "messageKey": "TextColor",
        "defaultValue": "0xFFFFFF",
        "label": "Testo"
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
        "label": "Scegli il font",
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
        "defaultValue": "Orientamento"
      },
      {
        "type": "radiogroup",
        "messageKey": "ScreenOrientation",
        "label": "Scegli l'orientamento",
        "defaultValue": "0",
        "options": [
          { "label": "Automatico (con doppio tap)", "value": "0" },
          { "label": "Sempre Verticale", "value": "1" },
          { "label": "Sempre Orizzontale (sinistra)", "value": "2" },
          { "label": "Sempre Orizzontale (destra)", "value": "3" }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Salva"
  }
];
