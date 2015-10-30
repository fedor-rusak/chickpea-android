"use strict";

function init(global) {
	global.cacheTexture = function(label, path) {
		process.natives.cacheTexture(label, path);
	}

	global.cacheTexturesInit = function() {
		global.cacheTexture('explosion', 'images/fireball.png');
	}

	var inputDataArray = [];

	global.addInput = function(data) {
		inputDataArray.push(data);
	}

	global.processInput = function() {
		if (inputDataArray.length != 0) {
			console.log("inputData length = " + inputDataArray.length + ", content:");
			for (var i = 0; i < inputDataArray.length; i++)
				console.log(JSON.stringify(inputDataArray[i]));

			//clear
			inputDataArray.splice(0, inputDataArray.length);
		}
	}

	global.render = function() {
		if (!global.cameraSet) {
			var data = process.natives.getScreenDimensions();
			console.log(JSON.stringify(data));
			process.natives.setCamera(0.0, 0.0, 5.0);
			global.cameraSet = true;
		}
		console.log(Date.now());
		process.natives.clearScreen(0.1, 0.2, 0.3);
		process.natives.render("explosion", -1.0, -1.0, 0.0);
		process.natives.render("explosion", 1.0, 1.0, 0.0);
	}
}

try {
    module.exports = init;
}
catch(e) {
    //used without module loader
}