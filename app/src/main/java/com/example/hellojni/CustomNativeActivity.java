package com.example.hellojni;

import android.os.Bundle;
import android.view.Window;
import android.view.View;
import android.view.KeyEvent;
import android.content.res.Configuration;
import android.util.Log;

public class CustomNativeActivity extends android.app.NativeActivity {

	static {
		System.loadLibrary("gnustl_shared");
		System.loadLibrary("jxcore");
		System.loadLibrary("jx-wrapper");
		System.loadLibrary("opengl-wrapper");
		System.loadLibrary("opensles-wrapper");
		System.loadLibrary("engine");
		System.loadLibrary("main");
	}

	private static void setVisibility(Window window) {
		window.getDecorView().setSystemUiVisibility(
			View.SYSTEM_UI_FLAG_LAYOUT_STABLE
			| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
			| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
			| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
			| View.SYSTEM_UI_FLAG_FULLSCREEN
			| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
	}


	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
	
		setVisibility(getWindow());
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);

		if (hasFocus) {
			setVisibility(getWindow());
		}
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event) {
		Log.d("ZZZ", "source " + event.getSource());

		if (event.getSource() != KeyEvent.KEYCODE_TV_MEDIA_CONTEXT_MENU && event.getKeyCode() == 4) {
			return false;
		}

		return super.dispatchKeyEvent(event);
	}

}