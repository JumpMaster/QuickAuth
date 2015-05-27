var MAX_OTP_COUNT = 16;
var MAX_LABEL_LENGTH = 20;
var MAX_KEY_LENGTH = 64;
var MAX_MESSAGE_RETRIES = 5;
var APP_VERSION = 22;

var otp_count = 0;
var aplite_theme = -1;
var basalt_colors = -1;
var font_style = -1;
var timezone_offset = 0;
var idle_timeout = 0;
var message_send_retries = 0;
var msg_data;
var debug = false;

function checkKeyStringIsValid(key) {
	if (debug)
		console.log("INFO: Key="+key);
	if (key === null) {
		if (debug)
			console.log("INFO: Key failed null test");
		return false;
	}
	
	var colonPosition = key.indexOf(":");
	
	if (colonPosition <= 0) {
		if (debug)
			console.log("INFO: Key failed colonPosition <= 0 test");
		return false;
	}
	
	if (colonPosition >= key.length-1) {
		if (debug)
			console.log("INFO: Key failed colonPosition >= key.length-1 test");
		return false;
	}
	
	return true;
}

function getWatchVersion() {
	// 1 = Pebble OG
	// 2 = Pebble Steel
	// 3 = Pebble Time
	// 3 = Pebble Basalt Emulator (currently Pebble Time)
	// 4 = Pebble Time Steel
	
	var watch_version = 1;

	if(Pebble.getActiveWatchInfo) {
		// Available for use!
		var watch_name = Pebble.getActiveWatchInfo().model;

		if (watch_name.indexOf("pebble_time_steel") >= 0) {
			watch_version = 4;
		} else if (watch_name.indexOf("pebble_time") >= 0) {
			watch_version = 3;
		} else if (watch_name.indexOf("qemu_platform_basalt") >= 0) {
			watch_version = 3;
		} else if (watch_name.indexOf("pebble_steel") >= 0) {
			watch_version = 2;
		}
	}
	
	return watch_version;
}

function loadLocalVariables() {
	otp_count = 0;
	for (var i=0; i<MAX_OTP_COUNT; i++)
	{
		var tempKey = getItem("secret_pair"+i);
		if (checkKeyStringIsValid(tempKey))
			otp_count++;
		else
			break;
	}

	aplite_theme = parseInt(getItem("theme"));
	basalt_colors = getItem("basalt_colors");
	font_style = parseInt(getItem("font_style"));
	idle_timeout = getItem("idle_timeout");
	timezone_offset = new Date().getTimezoneOffset();
	
	aplite_theme = !aplite_theme ? 0 : aplite_theme;
	basalt_colors = !basalt_colors ? "0000FFFFFFFF" : basalt_colors;
	font_style = !font_style ? 0 : font_style;
	idle_timeout = idle_timeout === null ? 300 : parseInt(idle_timeout);
}

function getItem(reference) {
	var item = localStorage.getItem(reference);
	
	if (debug)
		console.log("INFO: Loading item. REF:" + reference + " ITEM:" + item);
		
	return item;
}

function setItem(reference, item) {
	if (debug)
		console.log("INFO: Saving item. REF:" + reference + " ITEM:" + item);
		
	localStorage.setItem(reference ,item);
}

function sendAppMessage(data) {
	msg_data = data;
	Pebble.sendAppMessage(data,
						function(e) { // SUCCESS
							if (debug)
								console.log("INFO: Successfully delivered message with transactionId=" + e.data.transactionId);
							message_send_retries = 0;
						}, function(e) { // FAILURE
							if (debug)
								console.log("ERROR: Unable to deliver message with transactionId=" + e.data.transactionId);// + " Error is: " + e.error.message);
							if (message_send_retries <= MAX_MESSAGE_RETRIES) {
								message_send_retries++;
								sendAppMessage(msg_data);
							}
						});
}

Pebble.addEventListener("ready",
							function(e) {
								if (debug)
									console.log("INFO: JavaScript app ready and running!");

								// localStorage should only be accessed are the "ready" event is fired
								loadLocalVariables();
								
								setItem("version", APP_VERSION);
								
								// Send timezone, keycount, and theme to watch
								sendAppMessage({
									"key_count":otp_count, 
									"theme":aplite_theme,
									"basalt_colors":basalt_colors,
									"timezone":timezone_offset,
									"font_style":font_style,
									"idle_timeout":idle_timeout
									});

								if (debug) {
									console.log("INFO: otp_count="+otp_count);
									console.log("INFO: theme="+aplite_theme);
									console.log("INFO: basalt_colors="+basalt_colors);
									console.log("INFO: timezoneOffset="+timezone_offset);
									console.log("INFO: font_style="+font_style);
									console.log("INFO: idle_timeout="+idle_timeout);
									console.log("INFO: getWatchVersion()="+getWatchVersion());
								}

								// ####### CLEAN APP ##############
								//for (var i=0; i<MAX_OTP; i++)
								//{
								//	localStorage.removeItem('secret_pair'+i);
								//}
								//localStorage.removeItem("theme");
								//localStorage.removeItem("basalt_colors");
								//localStorage.removeItem("font_style");
								//localStorage.removeItem("idle_timeout");
								// ####### /CLEAN APP ##############*/
							}
						);

function sendKeyToWatch(secret) {
	sendAppMessage({"transmit_key":secret});
}

function confirmDelete(secret) {
	var blnFound = false;
	for (var i = 0; i < MAX_OTP_COUNT;i++) {
		var savedSecret = getItem('secret_pair'+i);
		
		if (savedSecret !== null && savedSecret.indexOf(secret) != -1)
			blnFound = true;
		
		if (blnFound) {
			var nextSecret = getItem('secret_pair'+(i+1));
			if (nextSecret)
				setItem('secret_pair'+i,nextSecret);
			else
				localStorage.removeItem('secret_pair'+i);
		}
	}
	sendAppMessage({"delete_key":secret});
}

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
							function(e) {
								if (debug)
									console.log("INFO: Message Recieved");
								if (e.payload.request_key) {
									if (debug)
										console.log("INFO: Requested key: "+e.payload.request_key);
									var requested_key = getItem("secret_pair"+(e.payload.request_key-1));
									if (checkKeyStringIsValid(requested_key))
										sendKeyToWatch(requested_key);
									else
										sendKeyToWatch("NULL"+e.payload.request_key);
								}
								else if (e.payload.delete_key) {
									if (debug)
										console.log("INFO: Deleting key: "+e.payload.delete_key);
									confirmDelete(e.payload.delete_key);
								}
								else {
									if (debug)
										console.log("INFO: Unknown payload:"+e.payload);
								}
							});

Pebble.addEventListener('showConfiguration', function(e) {
	sendAppMessage({"idle_timeout":-1});

		var url = 'http://oncloudvirtual.com/pebble/pebbleauth/v'+
		APP_VERSION+'/'+
		'?otp_count='+otp_count+
		'&theme='+aplite_theme+
		'&basalt_colors='+basalt_colors+
		'&font_style='+font_style+
		'&idle_timeout='+idle_timeout+
		'&watch_version='+getWatchVersion();

	if (debug)
		console.log("INFO: "+url);
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed",
							function(e) {
								var configuration = JSON.parse(decodeURIComponent(e.response));
								var config ={};
								var i = 0;

								if(!isNaN(configuration.delete_all)) {
									if (debug)
										console.log("INFO: Delete all requested");
									for (i = 0; i < MAX_OTP_COUNT;i++) {
										if (debug)
											console.log("INFO: Deleting key "+i);
										localStorage.removeItem('secret_pair'+i);
									}
									otp_count = 0;
									localStorage.removeItem("theme");
									localStorage.removeItem("basalt_colors");
									localStorage.removeItem("font_style");
									localStorage.removeItem("idle_timeout");
									sendAppMessage(configuration);
									return;
								}
								
								if(!isNaN(configuration.theme) && configuration.theme != aplite_theme) {
									if (debug)
										console.log("INFO: Theme changed");
									
									aplite_theme = configuration.theme;
									setItem("theme",aplite_theme);
									config.theme = aplite_theme;
								}
								
								if(configuration.basalt_colors && configuration.basalt_colors.length == 12 && configuration.basalt_colors.localeCompare(basalt_colors) !== 0) {
									if (debug)
										console.log("INFO: Colors changed");
									
									basalt_colors = configuration.basalt_colors;
									setItem("basalt_colors", basalt_colors);
									config.basalt_colors = basalt_colors;
								}
								
								if(!isNaN(configuration.font_style) && configuration.font_style != font_style) {
									if (debug)
										console.log("INFO: Font style changed:"+configuration.font_style);

									font_style = configuration.font_style;
									setItem("font_style",font_style);
									config.font_style = font_style;
								}
								
								if(!isNaN(configuration.idle_timeout) && configuration.idle_timeout != idle_timeout) {
									if (debug)
										console.log("INFO: Idle timeout changed");
									
									idle_timeout = configuration.idle_timeout;
									setItem("idle_timeout",idle_timeout);
								}
								// Always send idle_timeout to reset it after
								// it was disabled while configuring
								config.idle_timeout = idle_timeout;
								
								if(configuration.label && configuration.secret) {
									
									var secret = configuration.secret
										.replace(/0/g,"O")
										.replace(/1/g, "I")
										.replace(/\+/g, '')
										.replace(/\s/g, '')
										.toUpperCase()
										.substring(0, MAX_KEY_LENGTH);
									var label = configuration.label
										.replace(/:/g, '')
										.substring(0, MAX_LABEL_LENGTH);
									var secretPair = label + ":" + secret;
									
									var valid_key = checkKeyStringIsValid(secretPair);
									
									var blnKeyExists = false;
									for (i=0;i<otp_count;i++) {
										var savedSecret = getItem('secret_pair'+i);
										if (savedSecret !== null && savedSecret.indexOf(secret) != -1) {
											if (debug)
												console.log("INFO: Relabled code");
											
											blnKeyExists = true;
											setItem('secret_pair'+i,secretPair);
											config.transmit_key = secretPair;
										}
									}
									if(valid_key && !blnKeyExists && otp_count < MAX_OTP_COUNT) {
										if (debug)
											console.log("INFO: Uploading new key:"+secretPair);
										
										setItem('secret_pair'+otp_count,secretPair);
										otp_count++;
										config.transmit_key = secretPair;
									}
									else if (blnKeyExists) {
										if (debug)
											console.log("INFO: Code exists, updating label");
									}
									else if (otp_count >= MAX_OTP_COUNT) {
										if (debug)
											console.log("WARN: Too many codes..."+otp_count);
									}
								}
								if (debug)
									console.log("INFO: Uploading config");
								sendAppMessage(config);
							}
						);