var Clay       = require('pebble-clay');
var clayConfig = require('./config');
var clay       = new Clay(clayConfig);

// ---------- Clay settings helpers ----------

function readSettings() {
  var settings = {};
  try {
    var raw = localStorage.getItem('clay-settings');
    if (raw) {
      settings = JSON.parse(raw);
    }
  } catch (e) {
    console.log('Error parsing clay-settings: ' + e);
  }
  return settings;
}

function useCelsius() {
  var v = readSettings().USECELSIUS;
  return v === true || v === 1 || v === 'true';
}

// ---------- Weather (Open-Meteo, no API key required) ----------

// Open-Meteo describes conditions with WMO weather interpretation codes.
// Collapse them into short words that fit in a quadrant.
function conditionsFromWmoCode(code) {
  if (code === 0 || code === 1)   return 'Clear';    // clear / mainly clear
  if (code === 2 || code === 3)   return 'Clouds';   // partly cloudy / overcast
  if (code === 45 || code === 48) return 'Fog';
  if (code >= 51 && code <= 57)   return 'Drizzle';  // incl. freezing drizzle
  if (code >= 61 && code <= 67)   return 'Rain';     // incl. freezing rain
  if (code >= 71 && code <= 77)   return 'Snow';     // incl. snow grains
  if (code >= 80 && code <= 82)   return 'Showers';
  if (code === 85 || code === 86) return 'Snow';     // snow showers
  if (code >= 95 && code <= 99)   return 'Storm';    // thunderstorm, with/without hail
  return '--';
}

function xhrRequest(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    if (this.status !== 200) {
      console.log('Weather request failed: HTTP ' + this.status);
      return;
    }
    callback(this.responseText);
  };
  xhr.onerror = function() {
    console.log('Weather request failed: network error');
  };
  xhr.open('GET', url);
  xhr.send();
}

function locationSuccess(pos) {
  // ~100 m precision is plenty for weather and keeps the request short.
  var lat = pos.coords.latitude.toFixed(3);
  var lon = pos.coords.longitude.toFixed(3);
  var url = 'https://api.open-meteo.com/v1/forecast' +
            '?latitude=' + lat + '&longitude=' + lon +
            '&current=temperature_2m,weather_code';

  console.log('Weather URL: ' + url);

  xhrRequest(url, function(responseText) {
    var json;
    try {
      json = JSON.parse(responseText);
    } catch (e) {
      console.log('Weather response is not JSON: ' + e);
      return;
    }
    if (!json.current || typeof json.current.temperature_2m !== 'number') {
      console.log('Weather response has no current data: ' + responseText);
      return;
    }

    var tempC        = json.current.temperature_2m;  // degrees Celsius
    var temperatureC = Math.round(tempC);
    var temperatureF = Math.round(tempC * 1.8 + 32);
    var conditions   = conditionsFromWmoCode(json.current.weather_code);

    console.log('Temp: ' + temperatureC + 'C / ' + temperatureF + 'F');
    console.log('Conditions: ' + conditions + ' (WMO ' + json.current.weather_code + ')');

    var dictionary = {
      TEMPERATUREC: temperatureC,
      TEMPERATUREF: temperatureF,
      CONDITIONS:   conditions,
      USECELSIUS:   useCelsius() ? 1 : 0
    };

    Pebble.sendAppMessage(dictionary,
      function() { console.log('Weather sent to Pebble'); },
      function(e) { console.log('Error sending weather to Pebble: ' + e.error.message); }
    );
  });
}

function locationError(err) {
  console.log('Error requesting location: ' + JSON.stringify(err));
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    { timeout: 15000, maximumAge: 60000 }
  );
}

// ---------- Pebble events ----------

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready');
  getWeather();
});

// The watch sends an (empty) AppMessage every 30 minutes to ask for a refresh.
Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received: ' + JSON.stringify(e.payload));
  getWeather();
});
