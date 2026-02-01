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

        // If fetch fails, make sure the currently saved zone is still in the dropdown
        if (wasError && idstate.get()) {
            var selectElement = item.$manipulatorTarget[0] || item.$manipulatorTarget;
            var currentOptions = selectElement.options;
            var exists = false;
            
            // Check if the saved timezone already exists in the dropdown
            if (currentOptions && currentOptions.length) {
                for (var i = 0; i < currentOptions.length; i++) {
                    if (currentOptions[i].value === idstate.get()) {
                        exists = true;
                        break;
                    }
                }
            }
            
            // If not found, add it
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

            // Add all timezone options
            for (var i = 0; i < timezones.length; i++) {
                var option = document.createElement('option');
                option.value = timezones[i];
                option.textContent = timezones[i];
                selectElement.appendChild(option);
            }

            tzDebug = "Loaded " + timezones.length + " zones.";
            
            // Restore the previously selected timezone if it exists
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

        var url = 'http://worldtimeapi.org/api/timezone?v=' + Date.now();
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
                
                // Validate response
                if (!responseText || responseText.length < 10) {
                    throw new Error("Empty or invalid response");
                }
                
                // Try to parse to validate it's JSON
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

        // Set timeout
        timeoutId = setTimeout(function() {
            if (!requestStarted) {
                handleError("Request never started - possible browser blocking");
            } else {
                xhr.abort();
                handleError("Request timed out after 10 seconds");
            }
        }, 10000);

        xhr.onreadystatechange = function() {
            console.log("[TZ Debug] ReadyState changed to: " + xhr.readyState);
            if (xhr.readyState === 1) {
                requestStarted = true;
                updateDebug("Connection opened, sending request...");
            }
            if (xhr.readyState === 2) {
                updateDebug("Request sent, waiting for response...");
            }
            if (xhr.readyState === 3) {
                updateDebug("Receiving data...");
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
        
        xhr.onabort = function() {
            // Timeout triggered, error already handled
            console.log("[TZ Debug] Request aborted");
        };
        
        try {
            updateDebug("Opening connection to WorldTimeAPI...");
            xhr.open('GET', url, true);
            
            updateDebug("Sending request...");
            xhr.send();
        } catch (e) {
            handleError("Failed to send request: " + e.message);
        }
    };

    config.on(config.EVENTS.AFTER_BUILD, function () {
        built = true;
        console.log("[TZ Debug] Page built");

        var stateItem = config.getItemByMessageKey("TZ_ID_STATE");
        if (stateItem) stateItem.hide();

        var debug = config.getItemById("TZ_DEBUG");
        if (debug) {
            debug.show();
            debug.set("Click 'Fetch Timezones' to load the timezone list.");
        }

        var retryBtn = config.getItemById("TZ_BUTTON");
        if (retryBtn) {
            console.log("[TZ Debug] Retry button found, attaching click handler");
            retryBtn.on('click', function() {
                console.log("[TZ Debug] Retry button clicked");
                getTimezones();
            });
        } else {
            console.error("[TZ Debug] Retry button NOT found!");
        }

        // Do NOT auto-fetch - wait for user to click the button
    });
};