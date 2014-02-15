var otp_count = localStorage.getItem("otp_count");

Pebble.addEventListener("ready",
							function(e) {
								console.log("JavaScript app ready and running!");
								sendTimezoneToWatch();
								if (!isNaN(otp_count))
									otp_count = 0;
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
	Pebble.sendAppMessage({"transmit_key":secret});
}

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
							function(e) {
								console.log("Message Recieved");
								if (e.payload.timezone) {
									sendTimezoneToWatch();
								}
								if (e.payload.request_key) {
									console.log(e.payload.request_key);
									sendKeyToWatch(localStorage.getItem("secret_pair"+e.payload.request_key));
									otp_count += 1;
								}
								if (e.payload.key_count) {
									console.log("Key count request");
									Pebble.sendAppMessage({"send_key_count":otp_count});
								}
							});

Pebble.addEventListener('showConfiguration', function(e) {
	Pebble.openURL('http://192.168.0.115');
	//Pebble.openURL('http://htmlpreview.github.io/?https://github.com/JumpMaster/PebbleAuth/blob/master/configuration.html');
});

Pebble.addEventListener("webviewclosed",
							function(e) {
								console.log("Configuration window returned: " + e.response);
								if(otp_count < 8) {
									console.log("Uploading codes...");
									var configuration = JSON.parse(e.response);
									var secret = configuration.secret;
									secret = secret.replace(/0/g,"O").replace(/1/g, "I").replace(/\+/g, '').replace(/\s/g, '').toUpperCase();
									var secretPair = configuration.label + ":" + secret;
									console.log("Configuration window returned: " + secretPair);
									sendKeyToWatch(secretPair);
									console.log('secret_pair'+otp_count);
									localStorage.setItem('secret_pair'+otp_count,secretPair);
									otp_count++;
									localStorage.setItem('otp_count',otp_count);
								} else {
									console.log("Too many codes...");
								}
								
							}
						);