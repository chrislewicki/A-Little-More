var Clay       = require('pebble-clay');
var clayConfig = require('./config');
var clay       = new Clay(clayConfig);

var DEFAULT_API_KEY = 'YOU THOUGHT';

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

function getApiKey() {
  var s = readSettings();

  // Clay stores values keyed by messageKey, e.g. "APIKEY"
  if (s.APIKEY && s.APIKEY.length > 0) {
    console.log('Using Clay API key: ' + s.APIKEY);
    return s.APIKEY;
  }

  if (DEFAULT_API_KEY && DEFAULT_API_KEY.length > 0) {
    console.log('Using DEFAULT_API_KEY fallback');
    return DEFAULT_API_KEY;
  }

  console.log('No API key configured');
  return null;
}

function useCelsius() {
  var s = readSettings();
  var v = s.USECELSIUS;
  return v === true || v === 1 || v === 'true';
}

// ---------- Weather logic ----------

function xhrRequest(url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
}

function locationSuccess(pos) {
  var apiKey = getApiKey();
  if (!apiKey) {
    console.log('Aborting weather fetch: no API key');
    return;
  }

  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
            pos.coords.latitude + '&lon=' + pos.coords.longitude +
            '&appid=' + encodeURIComponent(apiKey);

  console.log('Weather URL: ' + url);

  xhrRequest(url, 'GET', function(responseText) {
    var json = JSON.parse(responseText);

    var temperatureC = Math.round(json.main.temp - 273.15);
    var temperatureF = Math.round(temperatureC * 1.8 + 32);
    var conditions   = json.weather[0].main;

    console.log('Temp: ' + temperatureC + 'C / ' + temperatureF + 'F');
    console.log('Conditions: ' + conditions);

    var dictionary = {
      TEMPERATUREC: temperatureC,
      TEMPERATUREF: temperatureF,
      CONDITIONS:   conditions,
      USECELSIUS:  useCelsius() ? 1 : 0,
      APIKEY:       apiKey
    };

    Pebble.sendAppMessage(dictionary,
      function() { console.log('Weather sent to Pebble'); },
      function(e) { console.log('Error sending weather to Pebble: ' + e.error.message); }
    );
  });

  console.log('Location: ' + JSON.stringify(pos));
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

// If you send an AppMessage from the watch to trigger refresh, this will refetch.
Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received: ' + JSON.stringify(e.payload));
  getWeather();
});