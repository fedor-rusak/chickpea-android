try {
	var registerAssets = function() {
		var fs = require('fs');

		var readfilesync = function(pathname) {
			return process.natives.assetReadSync("jxcore"+pathname);
		};

		var existssync = function(pathname) {
			var result = null, isFolder = false;
			if (isFolder) {
				result = {
					size: 340,
					mode: 16877,
					ino: fs.virtualFiles.getNewIno()
				};
			}
			else {
				result = {
					size: 0,
					mode: 33188,
					ino: fs.virtualFiles.getNewIno()
				};
			}

			return result;
		};

		var readdirsync = function(pathname) {
			//not used yet
			return null;
		}

		var extension = {
			readFileSync: readfilesync,
			readDirSync: readdirsync,
			existsSync: existssync,
		};

		fs.setExtension("jxcore-java", extension);
	}

	registerAssets();

	global.require = require;

	require("/init.js")(global);
}
catch(e) {
	console.log("init.js error: "+e);
}