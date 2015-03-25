	
		function queryObj() {
			var result = {}, keyValuePairs = location.search.slice(1).split('&');

			keyValuePairs.forEach(function(keyValuePair) {
				keyValuePair = keyValuePair.split('=');
				result[keyValuePair[0]] = keyValuePair[1] || '';
			});

			return result;
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
			console.log(options.idle_timeout);
			
			if ($("#keysecret").val()) {
				options.label =  $("#keyname").val();
				options.secret = $("#keysecret").val().replace(/\s/g, '');
			}

			return options;
		}

		$("document").ready(function() {

			$("#delete_all").click(function() {
				var options = {};
				options.delete_all = 1;
				var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(options));
				document.location = location;
			});
			
			$("#cancel").click(function() {
			  console.log("Cancel");
			  document.location = "pebblejs://close";
			});

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
					var location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(options));
					console.log("Warping to:");
					console.log(location);
					document.location = location;
				}
			});

			var otp_count = queryObj()["otp_count"];
			var slots_remaining = 0;
			var theme = queryObj()["theme"];
			var font_style = queryObj()["font_style"];
			var idle_timeout = queryObj()["idle_timeout"];
			
			theme = parseInt(isNumber(theme) && theme <= 1 ? theme : 0);
			font_style = parseInt(isNumber(font_style) && font_style <= 3 ? font_style : 0);
			idle_timeout = parseInt(isNumber(idle_timeout) && (idle_timeout >= 0 || idle_timeout == 60 || idle_timeout == 120 || idle_timeout == 300 || idle_timeout == 600) ? idle_timeout : 300);
			
			$('#theme_'+theme).prop( "checked", true ).checkboxradio("refresh");
			$('#font_style_'+font_style).prop( "checked", true ).checkboxradio("refresh");
			$("#idle_timeout").val(idle_timeout).selectmenu("refresh");

			otp_count = parseInt(isNumber(otp_count) ? otp_count : 0);
			slots_remaining = 16 - otp_count;
			
			if (slots_remaining === 0) {
				document.getElementById("keyname").disabled=true;
				document.getElementById("keysecret").disabled=true;
			}
			document.getElementById("footer").innerHTML=document.getElementById("footer").innerHTML.replace("NUM_SPACES", slots_remaining);
		});