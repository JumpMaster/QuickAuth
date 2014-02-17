var otp_count = parseInt(localStorage.getItem("otp_count"));
var theme = parseInt(localStorage.getItem("theme"));
var timezoneOffset = new Date().getTimezoneOffset();

if (!otp_count)
	otp_count = 0;

if (!theme)
	theme = 0;

Pebble.addEventListener("ready",
							function(e) {
								console.log("JavaScript app ready and running!");
								
								//otp_count = 4;
								//localStorage.setItem("otp_count", otp_count);
								//localStorage.removeItem("secret4");
								//localStorage.removeItem("secret6");
								//localStorage.removeItem("secret7");

								// Send timezone and keycount to watch
								Pebble.sendAppMessage({"key_count":otp_count, "theme":theme, "timezone":timezoneOffset});
								
								console.log("otp_count="+otp_count);
								console.log("theme="+theme);
								console.log("timezoneOffset="+timezoneOffset);
							}
						);

function sendKeyToWatch(secret) {
	Pebble.sendAppMessage({"transmit_key":secret});
}

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
							function(e) {
								console.log("Message Recieved");
								if (e.payload.request_key) {
									console.log(e.payload.request_key);
									sendKeyToWatch(localStorage.getItem("secret_pair"+(e.payload.request_key-1)));
								}
								else {
									console.log(e.payload);
								}
							});

Pebble.addEventListener('showConfiguration', function(e) {
	Pebble.openURL('http://192.168.4.17');
	//Pebble.openURL('http://htmlpreview.github.io/?https://github.com/JumpMaster/PebbleAuth/blob/master/html/configuration.html');
});

Pebble.addEventListener("webviewclosed",
							function(e) {
								var configuration = JSON.parse(e.response);
								var config ={};
								
								if(!isNaN(configuration.theme) && configuration.theme != theme) {
									theme = configuration.theme;
									localStorage.setItem("theme",theme);
									config.theme = theme;
									console.log("Theme changed...");
								}
								
								if(otp_count < 8) {
									if(configuration.label.length > 0 && configuration.secret.length > 0) {
										console.log("Uploading codes...");
										var secret = configuration.secret;
										secret = secret.replace(/0/g,"O").replace(/1/g, "I").replace(/\+/g, '').replace(/\s/g, '').toUpperCase();
										
										// Check if secret exists
										var blnKeyExists = false;
										for (var i=0;i<otp_count;i++) {
											var savedSecret = localStorage.getItem('secret_pair'+i);
											if (savedSecret !== null && savedSecret.indexOf(secret) != -1)
												blnKeyExists = true;
										}
										
										if (!blnKeyExists) {
											var secretPair = configuration.label + ":" + secret;
											console.log("Uploading new key:"+secretPair);
											localStorage.setItem('secret_pair'+otp_count,secretPair);
											otp_count++;
											localStorage.setItem("otp_count",otp_count);
											config.transmit_key = secretPair;
										} else
											console.log("Key exists, ignoring");
									}
								} else
									console.log("Too many codes...");

								Pebble.sendAppMessage(config);
							}
						);