/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId])
/******/ 			return installedModules[moduleId].exports;
/******/
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			exports: {},
/******/ 			id: moduleId,
/******/ 			loaded: false
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.loaded = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

	__webpack_require__(1);
	module.exports = __webpack_require__(2);


/***/ }),
/* 1 */
/***/ (function(module, exports) {

	/**
	 * Copyright 2024 Google LLC
	 *
	 * Licensed under the Apache License, Version 2.0 (the "License");
	 * you may not use this file except in compliance with the License.
	 * You may obtain a copy of the License at
	 *
	 *     http://www.apache.org/licenses/LICENSE-2.0
	 *
	 * Unless required by applicable law or agreed to in writing, software
	 * distributed under the License is distributed on an "AS IS" BASIS,
	 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	 * See the License for the specific language governing permissions and
	 * limitations under the License.
	 */
	
	(function(p) {
	  if (!p === undefined) {
	    console.error('Pebble object not found!?');
	    return;
	  }
	
	  // Aliases:
	  p.on = p.addEventListener;
	  p.off = p.removeEventListener;
	
	  // For Android (WebView-based) pkjs, print stacktrace for uncaught errors:
	  if (typeof window !== 'undefined' && window.addEventListener) {
	    window.addEventListener('error', function(event) {
	      if (event.error && event.error.stack) {
	        console.error('' + event.error + '\n' + event.error.stack);
	      }
	    });
	  }
	
	})(Pebble);


/***/ }),
/* 2 */
/***/ (function(module, exports) {

	var APIKey = '5d1c86322d54de28267b99514cc875a3';
	
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

/***/ })
/******/ ]);
//# sourceMappingURL=pebble-js-app.js.map