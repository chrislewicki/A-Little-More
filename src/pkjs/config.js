module.exports = [
  {
    type: 'heading',
    defaultValue: 'A Little More'
  },
  {
    type: 'section',
    items: [
      {
        type: 'toggle',
        messageKey: 'USECELSIUS',
        label: 'Show temperature in Celsius',
        defaultValue: false   // false = Fahrenheit default
      },
      {
        type: 'input',
        messageKey: 'APIKEY',
        label: 'OpenWeatherMap API key',
        defaultValue: '',
        attributes: {
          placeholder: 'Your OWM API key',
          type: 'text'
        }
      },
      {
        "type": "text",
        "defaultValue": "If you do not provide your own key, a default one will be used. However, the default key has a rather low number of allowed calls per day. If temperature readings aren't working for you, please specify your own key."
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'submit',
        defaultValue: 'Save'
      }
    ]
  }
];