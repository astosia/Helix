module.exports = function(minified) {
    var config = this;
    var $ = minified.$;
    var HTML = minified.HTML;

    var timezonesJSON = 0; // 0 is unset
    var built = false;
    var tzDebug = "Ready.";

    var postBuild = function(wasError) {
        var item = config.getItemByMessageKey("TZ_ID");
        var idstate = config.getItemByMessageKey("TZ_ID_STATE");
        var debug = config.getItemById("TZ_DEBUG");

        if (!item || !idstate) return;

        if (wasError && idstate.get()) {
            var selectElement = item.$manipulatorTarget[0] || item.$manipulatorTarget;
            var currentOptions = selectElement.options;
            var exists = false;
            
            if (currentOptions && currentOptions.length) {
                for (var i = 0; i < currentOptions.length; i++) {
                    if (currentOptions[i].value === idstate.get()) {
                        exists = true;
                        break;
                    }
                }
            }
            
            if (!exists) {
                var option = document.createElement('option');
                option.value = idstate.get();
                option.textContent = idstate.get();
                selectElement.appendChild(option);
            }
        }

        if (debug) {
            if (wasError) {
                debug.show();
            } else {
                debug.hide();
            }
            debug.set(tzDebug + "<br>Click 'Fetch Timezones' to retry.");
        }

        if (idstate.get()) {
            item.set(idstate.get());
        }

        item.on('change', function() {
            idstate.set(item.get());
        });
    };

    var loadTimezones = function(json) {
        var item = config.getItemByMessageKey("TZ_ID");
        var idstate = config.getItemByMessageKey("TZ_ID_STATE");
        
        try {
            var timezones = JSON.parse(json);
            
            var selectElement = item.$manipulatorTarget[0] || item.$manipulatorTarget;
            selectElement.innerHTML = '';
            
            var disabledOption = document.createElement('option');
            disabledOption.value = '';
            disabledOption.textContent = 'Disabled';
            selectElement.appendChild(disabledOption);

            for (var i = 0; i < timezones.length; i++) {
                var option = document.createElement('option');
                option.value = timezones[i];
                option.textContent = timezones[i];
                selectElement.appendChild(option);
            }

            tzDebug = "Loaded " + timezones.length + " zones.";
            
            if (idstate && idstate.get()) {
                item.set(idstate.get());
            }
            
            postBuild(false);
        } catch (e) {
            tzDebug = "JSON Parse Error: " + e.message;
            postBuild(true);
        }
    };

    var updateDebug = function(message) {
        console.log("[TZ Debug] " + message);
        tzDebug = message;
        var debug = config.getItemById("TZ_DEBUG");
        if (debug) {
            debug.show();
            debug.set(message);
        }
    };

    var getTimezones = function() {
        updateDebug("Starting fetch...");

        // Updated to timeapi.io AvailableTimeZones endpoint
        var url = 'https://timeapi.io/api/TimeZone/AvailableTimeZones';
        var xhr = new XMLHttpRequest();
        var timeoutId;
        var requestStarted = false;

        var cleanup = function() {
            if (timeoutId) {
                clearTimeout(timeoutId);
                timeoutId = null;
            }
        };

        var handleError = function(errorMsg) {
            cleanup();
            console.error("Timezone fetch error:", errorMsg);
            tzDebug = "Error: " + errorMsg;
            timezonesJSON = null;
            if (built) {
                postBuild(true);
            }
        };

        var handleSuccess = function(responseText) {
            cleanup();
            try {
                updateDebug("Processing response...");
                
                if (!responseText || responseText.length < 10) {
                    throw new Error("Empty or invalid response");
                }
                
                var parsed = JSON.parse(responseText);
                if (!parsed || parsed.length === 0) {
                    throw new Error("Invalid timezone data");
                }
                
                timezonesJSON = responseText;
                if (built) {
                    loadTimezones(responseText);
                }
            } catch (e) {
                handleError(e.message);
            }
        };

        timeoutId = setTimeout(function() {
            if (!requestStarted) {
                handleError("Request never started - possible browser blocking");
            } else {
                xhr.abort();
                handleError("Request timed out after 10 seconds");
            }
        }, 10000);

        xhr.onreadystatechange = function() {
            if (xhr.readyState === 1) {
                requestStarted = true;
                updateDebug("Connection opened, sending request...");
            }
        };

        xhr.onload = function() {
            updateDebug("Response received (HTTP " + xhr.status + ")");
            if (xhr.status === 200) {
                handleSuccess(xhr.responseText);
            } else {
                handleError("HTTP " + xhr.status + ": " + xhr.statusText);
            }
        };
        
        xhr.onerror = function() {
            handleError("Network error - connection blocked");
        };
        
        try {
            updateDebug("Opening connection to TimeAPI.io...");
            xhr.open('GET', url, true);
            updateDebug("Sending request...");
            xhr.send();
        } catch (e) {
            handleError("Failed to send request: " + e.message);
        }
    };

    config.on(config.EVENTS.AFTER_BUILD, function () {
        built = true;
        var stateItem = config.getItemByMessageKey("TZ_ID_STATE");
        if (stateItem) stateItem.hide();

        var debug = config.getItemById("TZ_DEBUG");
        if (debug) {
            debug.show();
            debug.set("Click 'Fetch Timezones' to load the timezone list.");
        }

        var retryBtn = config.getItemById("TZ_BUTTON");
        if (retryBtn) {
            retryBtn.on('click', function() {
                getTimezones();
            });
        }
    });
};