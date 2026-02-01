var TZ_KEY = 'TZ_KEY';

var timezones = localStorage.getItem(TZ_KEY);
if (timezones !== null) {
    timezones = JSON.parse(timezones);
}

var indexOf = function(timezone) {
    return new Promise(function(resolve, reject) {
        var xhr = new XMLHttpRequest();
        xhr.onload = function () {
            var newTimezones = JSON.parse(this.responseText);
            localStorage.setItem(TZ_KEY, this.responseText);
            timezones = newTimezones;
            resolve(timezones.indexOf(timezone));
        };
        xhr.onerror = reject;
        xhr.open('GET', 'http://worldtimeapi.org/api/timezone?v=' + Date.now());
        xhr.send();
    });
};

// Fetch timezones from WorldTimeAPI
var fetchFromWorldTimeAPI = function(timezone) {
    return new Promise(function(resolve, reject) {
        var url = 'http://worldtimeapi.org/api/timezone/' + timezone + '?v=' + Date.now();
        var xhr = new XMLHttpRequest();
        var timeoutId = setTimeout(function() {
            xhr.abort();
            reject(new Error("Timezone offset request timed out"));
        }, 8000);
        
        xhr.onload = function() {
            clearTimeout(timeoutId);
            if (xhr.status === 200) {
                try {
                    var response = JSON.parse(xhr.responseText);
                    var keys = require('message_keys');
                    var code = '+-'.indexOf(response.abbreviation[0]) !== -1
                        ? 'UTC' + response.abbreviation : response.abbreviation;
                    var result = {};
                    result[keys.TZ_OFFSET] = response.raw_offset + response.dst_offset;
                    result[keys.TZ_CODE] = code;
                    resolve(result);
                } catch (e) {
                    reject(new Error("Failed to parse timezone response"));
                }
            } else {
                reject(new Error('Timezone GET failed with error ' + xhr.status + ' ' + xhr.statusText));
            }
        };
        
        xhr.onerror = function() {
            clearTimeout(timeoutId);
            reject(new Error("Failed to make timezone GET request"));
        };
        
        xhr.open('GET', url);
        xhr.send();
    });
};

// Calculates offset locally first, fall back to API
var getTimezoneOffset = function(timezone) {
    return new Promise(function(resolve, reject) {
        try {
            // Method 1: Calculate offset using browser's Date API
            var now = new Date();
            
            // Get the current time in the target timezone
            var tzString = now.toLocaleString('en-US', { timeZone: timezone });
            var tzDate = new Date(tzString);
            
            // Get the current time in UTC
            var utcDate = new Date(now.toLocaleString('en-US', { timeZone: 'UTC' }));
            
            // Calculate the difference in seconds
            var offsetMs = tzDate.getTime() - utcDate.getTime();
            var offsetSeconds = Math.round(offsetMs / 1000);
            
            // Get timezone abbreviation
            var formatter = new Intl.DateTimeFormat('en-US', {
                timeZone: timezone,
                timeZoneName: 'short'
            });
            
            var parts = formatter.formatToParts(now);
            var tzNamePart = null;
            for (var i = 0; i < parts.length; i++) {
                if (parts[i].type === 'timeZoneName') {
                    tzNamePart = parts[i];
                    break;
                }
            }
            
            var code = tzNamePart ? tzNamePart.value : timezone.split('/').pop();
            
            // Validate the result makes sense (offset between -12 and +14 hours)
            if (offsetSeconds < -43200 || offsetSeconds > 50400) {
                throw new Error("Calculated offset out of valid range");
            }
            
            var keys = require('message_keys');
            var result = {};
            result[keys.TZ_OFFSET] = offsetSeconds;
            result[keys.TZ_CODE] = code;
            
            console.log("[TZ] Local calculation succeeded for " + timezone + ": " + offsetSeconds + "s (" + code + ")");
            resolve(result);
            
        } catch (e) {
            // If browser calculation fails, try WorldTimeAPI as fallback
            console.log("[TZ] Local calculation failed for " + timezone + ", trying API: " + e.message);
            fetchFromWorldTimeAPI(timezone)
                .then(function(result) {
                    console.log("[TZ] API fallback succeeded for " + timezone);
                    resolve(result);
                })
                .catch(function(err) {
                    console.error("[TZ] Both local and API methods failed for " + timezone);
                    reject(err);
                });
        }
    });
};

var getTimezoneOffsetFromIndex = function(index) {
    return getTimezoneOffset(timezones[index]);
};

module.exports = {
    indexOf: indexOf,
    get: getTimezoneOffset,
    getFromIndex: getTimezoneOffsetFromIndex
};