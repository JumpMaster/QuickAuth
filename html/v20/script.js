$(function () {
  $('#foreground-color').on('click', function (event) {
	changeForeground = true;
	highlightColorBox($(this));
  });
});

$(function () {
  $('#background-color').on('click', function (event) {
	changeForeground = false;
	highlightColorBox($(this));
  });
});

$(function () {
  $('#color-picker .color-box').on('click', function (event) {
	var selected_color = rgb2hex($(this).css("background-color"));

	if (changeForeground)
		$('#foreground-color').css('background-color', selected_color);
	else
		$('#background-color').css('background-color', selected_color);
	$( "#color-picker" ).popup( "close" );   
  });
});

$(function () {
	$("#confirm_delete_all").click(function() {
		console.log("Deleting all");
		var options = {};
		options.delete_all = 1;
		var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(options));
		document.location = location;
	});
});

$(function () {
	$("#cancel").click(function() {
		console.log("Cancelling");
		document.location = "pebblejs://close";
	});
});

$(function () {
	$("#save").click(function() {
		console.log("Submit");

		var options = saveOptions();
			  
		if (options.secret && base32length(options.secret) < 10) {
			alert("Secret key value is too short");
		}
		else if (options.secret && options.label.length === 0) {
			alert("Please enter a label");
		}
		else {
			var return_to = getQueryParam('return_to', 'pebblejs://close#');
			var location = return_to + encodeURIComponent(JSON.stringify(options));
			console.log("Warping to:");
			console.log(location);
			document.location = location;
		}
	});
});

function highlightColorBox($colorButton) {
	selectedColor = rgb2hex($colorButton.css("background-color"));
	
	var i;
	for (i = 1; i <= 13; i++)
	{
		$('#color-row'+i).children().removeClass("selected-color-box");
	}
	$('#color-box-'+selectedColor.substring(1).toUpperCase()).addClass("selected-color-box");
}

//Function to convert hex format to a rgb color
function rgb2hex(rgb){
 rgb = rgb.match(/^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/);
 return ("#" +
  ("0" + parseInt(rgb[1],10).toString(16)).slice(-2) +
  ("0" + parseInt(rgb[2],10).toString(16)).slice(-2) +
  ("0" + parseInt(rgb[3],10).toString(16)).slice(-2)).toUpperCase();
}

function queryObj() {
	var result = {}, keyValuePairs = location.search.slice(1).split('&');

	keyValuePairs.forEach(function(keyValuePair) {
		keyValuePair = keyValuePair.split('=');
		result[keyValuePair[0]] = keyValuePair[1] || '';
	});

	return result;
}

function getQueryParam(variable, default_) {
    var query = location.search.substring(1);
    var vars = query.split('&');
    for (var i = 0; i < vars.length; i++) {
        var pair = vars[i].split('=');
        if (pair[0] == variable)
            return decodeURIComponent(pair[1]);
    }
    return default_ || false;
}

function isNumber(n) {
	return !isNaN(parseFloat(n)) && isFinite(n);
}

// Base32 decoder from https://github.com/agnoster/base32-js/blob/master/lib/base32.js by Isaac Wolkerstorfer
var alphabet = '0123456789abcdefghjkmnpqrtuvwxyz'
var alias = { o:0, i:1, l:1, s:5 }

var lookup = function() {
	var table = {}
	// Invert 'alphabet'
	for (var i = 0; i < alphabet.length; i++) {
		table[alphabet[i]] = i
	}
	// Splice in 'alias'
	for (var key in alias) {
		if (!alias.hasOwnProperty(key)) continue
		table[key] = table['' + alias[key]]
	}
	lookup = function() { return table }
	return table
}

function Decoder() {
	var skip = 0 // how many bits we have from the previous character
	var byte = 0 // current byte we're producing

	this.output = ''

	// Consume a character from the stream, store
	// the output in this.output. As before, better
	// to use update().
	this.readChar = function(char) {
		if (typeof char != 'string'){
			if (typeof char == 'number') {
				char = String.fromCharCode(char)
			}
		}
		char = char.toLowerCase()
		var val = lookup()[char]
		if (typeof val == 'undefined') {
			// character does not exist in our lookup table
			return // skip silently. An alternative would be:
			// throw Error('Could not find character "' + char + '" in lookup table.')
		}
		val <<= 3 // move to the high bits
		byte |= val >>> skip
		skip += 5
		if (skip >= 8) {
			// we have enough to preduce output
			this.output += String.fromCharCode(byte)
			skip -= 8
			if (skip > 0) byte = (val << (5 - skip)) & 255
			else byte = 0
		}

	}

	this.finish = function(check) {
		var output = this.output + (skip < 0 ? alphabet[bits >> 3] : '') + (check ? '$' : '')
		this.output = ''
		return output
	}
}

Decoder.prototype.update = function(input, flush) {
	for (var i = 0; i < input.length; i++) {
		this.readChar(input[i])
	}
	var output = this.output
	this.output = ''
	if (flush) {
	  output += this.finish()
	}
	return output
}

// Base32-encoded string goes in, decoded data comes out.
function decode(input) {
	var decoder = new Decoder()
	var output = decoder.update(input, true)
	return output
}

function base32length(secret) {
	return decode(secret).length;
}

function saveOptions() {

	var options = {};
	
	options.theme = parseInt($("input[name=theme]:checked").val(), 10);
	options.font_style = parseInt($("input[name=font_style]:checked").val(), 10);
	options.idle_timeout = parseInt($("#idle_timeout").val(), 10);
	
	var colors = rgb2hex($('#background-color').css("background-color")).replace("#", '');
	colors = colors + rgb2hex($('#foreground-color').css("background-color")).replace("#", '');
	options.basalt_colors = colors;
	
	if ($("#keysecret").val()) {
		options.label =  $("#keyname").val();
		options.secret = $("#keysecret").val().replace(/\s/g, '');
	}

	return options;
}

$("document").ready(function() {
	var watch_version = getQueryParam("watch_version", 1);
	
	if (watch_version >= 3) {
		var basalt_colors = getQueryParam("basalt_colors", 0);
		basalt_colors = basalt_colors.length == 12 ? basalt_colors : "00AAFFFFFFFF";
		$('#background-color').css('background-color', "#"+basalt_colors.substring(0,6));
		$('#foreground-color').css('background-color', "#"+basalt_colors.substring(6,12));
		$('#time-foreground').removeClass("hidden");
		$('#time-background').removeClass("hidden");
	} else {
		var theme = getQueryParam("theme", 0);
		theme = parseInt(isNumber(theme) && theme <= 1 ? theme : 0);
		$('#theme').removeClass("hidden");
		$('#theme_'+theme).prop( "checked", true ).checkboxradio("refresh");
	}
	
	var otp_count = getQueryParam("otp_count", 0);
	var font_style = getQueryParam("font_style", 0);
	var idle_timeout = getQueryParam("idle_timeout", 300);
	
	font_style = parseInt(isNumber(font_style) && font_style <= 3 ? font_style : 0);
	idle_timeout = parseInt(isNumber(idle_timeout) && (idle_timeout >= 0 || idle_timeout == 60 || idle_timeout == 120 || idle_timeout == 300 || idle_timeout == 600) ? idle_timeout : 300);

	$('#font_style_'+font_style).prop( "checked", true ).checkboxradio("refresh");
	$("#idle_timeout").val(idle_timeout).selectmenu("refresh");

	otp_count = parseInt(isNumber(otp_count) ? otp_count : 0);
	var slots_remaining = 16 - otp_count;
	
	if (slots_remaining === 0) {
		document.getElementById("keyname").disabled=true;
		document.getElementById("keysecret").disabled=true;
	}
	document.getElementById("footer").innerHTML=document.getElementById("footer").innerHTML.replace("NUM_SPACES", slots_remaining);
});