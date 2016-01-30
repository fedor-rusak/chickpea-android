"use strict";

function init(global, natives) {
	global.cacheTexture = function(label, path) {
		natives.cacheTexture(label, path);
	}

	global.cacheTexturesInit = function() {
		global.cacheTexture('explosion', 'images/with_alpha/explosion.png');
	}

	global.cacheSoundsInit = function() {
		natives.cacheSound('background', 'sound/background.mp3');
		natives.cacheSound('action', 'sound/sfx.wav');
	}

	var inputDataArray = [];

	global.addInput = function(data) {
		inputDataArray.push(data);
	}

	global.processInput = function() {
		if (inputDataArray.length != 0) {
			console.log("inputData length = " + inputDataArray.length + ", content:");
			for (var i = 0; i < inputDataArray.length; i++) {
				console.log(JSON.stringify(inputDataArray[i]));
				if (inputDataArray[i][0] !== "release" ) {
					console.log(natives.unproject(inputDataArray[i][2], inputDataArray[i][3]));
					if (inputDataArray[i][2] < 500 && !global.soundPlayed) {
						global.soundPlayed = true;
						natives.playSound();
					}
				}
			}

			//clear
			inputDataArray.splice(0, inputDataArray.length);
		}
	}

	global.render = function() {
		if (!global.cameraSet) {
			var data = natives.getScreenDimensions();
			console.log(JSON.stringify(data));
			natives.setCamera(0.0, 0.0, 5.0);
			global.cameraSet = true;
			global.radius = 0;
			global.angle = 0;
			global.step = 0.005;
			global.sign = +1;
		}
		var radians = global.angle*Math.PI/180;
		natives.setCamera(Math.sin(radians)*global.radius, Math.cos(radians)*global.radius, 5.0);

		global.radius += global.sign*global.step;
		global.angle += 1;
		if (global.angle == 400) {
			global.sign *=-1;
			global.angle = 40;
		}

		Math.sin(90*Math.PI/180)
		//track time with this
		// console.log(Date.now());

		natives.clearScreen(0.1, 0.2, 0.3);
		natives.render("explosion", -1.0, -1.0, 0.0);
		natives.render("explosion", 1.0, 1.0, 0.0);
	}
}

try {
    module.exports = init;
}
catch(e) {
    //used without module loader
}