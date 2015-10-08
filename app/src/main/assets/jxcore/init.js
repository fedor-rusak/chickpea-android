"use strict";

function init(global) {
	global.cacheTextures = function() {
		console.log("ZZZ!!!");
		process.natives.cacheTexture("explosion", "images/fireball.png");
	}
}

try {
    module.exports = init
}
catch(e) {
    //used without module loader
}