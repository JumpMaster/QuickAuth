//var symbol = localStorage.getItem("symbol");
Pebble.addEventListener("ready",
							function(e) {
							console.log("JavaScript app ready and running!");
							sendTimezoneToWatch();
							}
						);

function sendTimezoneToWatch() {
	// Get the number of seconds to add to convert localtime to utc
	var offsetMinutes = new Date().getTimezoneOffset();
	// Send it to the watch
	console.log(offsetMinutes);
	Pebble.sendAppMessage({"timezone":offsetMinutes});
}

function sendKeyToWatch(secret) {
	Pebble.sendAppMessage({"send_key":secret});
}

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
							function(e) {
								console.log("Message Recieved");
								if (e.payload.timezone) {
									sendTimezoneToWatch();
								}
								if (e.payload.request_key) {
									sendKeyToWatch(localStorage.getItem("key1"));
								}
							});

Pebble.addEventListener('showConfiguration', function(e) {
	Pebble.openURL('http://kevin-pc.carrenza.co.uk/index.html');
});

Pebble.addEventListener("webviewclosed",
	function(e) {
		console.log("Configuration window returned: " + e.response);
		var configuration = JSON.parse(e.response);
		var secret = configuration.label + ":" + configuration.secret;
		console.log("Configuration window returned: " + secret);
		sendKeyToWatch(secret);
		localStorage.setItem('key1',secret);
  }
);