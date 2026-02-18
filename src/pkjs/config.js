var quadrantOptions = [
  { label: 'None',         value: '0'  },
  { label: 'Battery',      value: '1'  },
  { label: 'Temperature',  value: '2'  },
  { label: 'Day of week',  value: '3'  },
  { label: 'Date',         value: '4'  },
  { label: 'Conditions',   value: '5'  },
  { label: 'Steps',        value: '6'  },
  { label: 'Distance',     value: '7'  },
  { label: 'Active time',  value: '8'  },
  { label: 'Heart rate',   value: '9'  },
  { label: 'Seconds',      value: '10' },
];

module.exports = [
  {
    type: 'heading',
    defaultValue: 'A Little More'
  },

  // ── Quadrant layout ──────────────────────────────────────────────────────
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Quadrants',
        size: 4
      },
      {
        type: 'select',
        messageKey: 'QUAD_TL',
        label: 'Top left',
        defaultValue: '1',
        options: quadrantOptions
      },
      {
        type: 'select',
        messageKey: 'QUAD_TR',
        label: 'Top right',
        defaultValue: '2',
        options: quadrantOptions
      },
      {
        type: 'select',
        messageKey: 'QUAD_BL',
        label: 'Bottom left',
        defaultValue: '3',
        options: quadrantOptions
      },
      {
        type: 'select',
        messageKey: 'QUAD_BR',
        label: 'Bottom right',
        defaultValue: '4',
        options: quadrantOptions
      },
      {
        type: 'text',
        defaultValue: 'Steps, distance, active time, and heart rate require a watch that supports Pebble Health. Heart rate also requires a watch with a heart rate sensor.'
      }
    ]
  },

  // ── Units ────────────────────────────────────────────────────────────────
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Units',
        size: 4
      },
      {
        type: 'toggle',
        messageKey: 'USECELSIUS',
        label: 'Temperature in Celsius',
        defaultValue: false
      },
      {
        type: 'toggle',
        messageKey: 'USEMETRIC',
        label: 'Distance in kilometers',
        defaultValue: false
      }
    ]
  },

  // ── Weather API ──────────────────────────────────────────────────────────
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Weather',
        size: 4
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
        type: 'text',
        defaultValue: 'If you do not provide your own key, a default one will be used. The default key has a low daily call limit; if temperature readings stop working, get a free key at openweathermap.org.'
      }
    ]
  },

  // ── Save ─────────────────────────────────────────────────────────────────
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
