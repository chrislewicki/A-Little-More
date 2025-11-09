var APIKey = 'HARHAR YOU THOUGHT';

var xhrRequest = function (url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {
    var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + 
        pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + APIKey;
    
    xhrRequest(url, 'GET', function(responseText) {
        var json = JSON.parse(responseText);

        // Temp is given in Kelvin, normalize to Celsius
        var temperatureC = Math.round(json.main.temp - 273.15);
        var temperatureF = Math.round((temperatureC * 1.8) + 32);
        console.log('Temperature is ' + temperatureC + ' Celsius, or ' + temperatureF + ' Fahrenheit');
        
        var conditions = json.weather[0].main;
        console.log('Conditions are ' + conditions);

        var dictionary = {
            'TEMPERATUREC': temperatureC,
            'TEMPERATUREF': temperatureF,
            'CONDITIONS': conditions
        };

        Pebble.sendAppMessage(dictionary, function(e) {
            console.log('Weather info successfully sent to Pebble!');
        }, function(e) {
            console.log('Error sending weather info to Pebble!');
        });
    });
    console.log(JSON.stringify(pos));
}

function locationError(err) {
    console.log('Error requesting location!');
}

function getWeather() {
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError,
        {timeout: 15000, maximumAge: 60000}
    );
}

Pebble.addEventListener('ready', function(e) {
    console.log('PebbleKit JS ready!');
    getWeather();
});

Pebble.addEventListener('appmessage', function(e) {
    console.log('AppMessage received!');
    getWeather();
});