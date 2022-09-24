
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0700
#include <dinput.h>


class UserInput
{
	public:
		UserInput();
		~UserInput();
		virtual bool isOK();
		virtual void release();

		virtual bool initMouse(const HWND hwnd);
		virtual bool updateMouse();
		DIMOUSESTATE mouseState;

		virtual bool initKeyboard(const HWND hwnd);
		virtual bool updateKeyboard();
		unsigned char keyState[256];

		virtual bool initJoystick(const HWND hwnd, const int minRange = -1000, const int maxRange = 1000);
		virtual bool updateJoystick();
		DIJOYSTATE joyState;

	private:
		bool good;
		IDirectInput * pDI;
		IDirectInputDevice * mouse;
		IDirectInputDevice * keyboard;
		IDirectInputDevice2 * joystick;
};
