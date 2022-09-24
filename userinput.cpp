
#define STRICT
#include "UserInput.h"
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dinput.lib")


UserInput::UserInput()
{
	good = false;

	pDI = 0;
	mouse = 0;
	keyboard = 0;
	joystick = 0;

	if (DirectInputCreate(GetModuleHandle(0), DIRECTINPUT_VERSION, &pDI, 0) != DI_OK)
		return;

	good = true;
}

UserInput::~UserInput()
{
	if (pDI)
	{
		if (mouse)
		{
			mouse->Unacquire();
			mouse->Release();
		}
		if (keyboard)
		{
			keyboard->Unacquire();
			keyboard->Release();
		}
		if (joystick)
		{
			joystick->Unacquire();
			joystick->Release();
		}
		pDI->Release();
	}
}

bool UserInput::isOK()
{
	return good;
}

void UserInput::release()
{
	delete this;
}

bool UserInput::initMouse(const HWND hwnd)
{
	good = false;

	if (pDI->CreateDevice(GUID_SysMouse, &mouse, 0) != DI_OK)
		return false;
	if (mouse->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) != DI_OK)
		return false;
	if (mouse->SetDataFormat(&c_dfDIMouse) != DI_OK)
		return false;
	HRESULT hr = mouse->Acquire();
	if (hr != DI_OK && hr != S_FALSE)
		return false;

	good = true;
	return true;
}

bool UserInput::updateMouse()
{
	good = false;
	if (mouse->GetDeviceState(sizeof DIMOUSESTATE, (LPVOID)&mouseState) != DI_OK)
		return false;
	good = true;
	return true;
}


bool UserInput::initKeyboard(const HWND hwnd)
{
	good = false;

	if (pDI->CreateDevice(GUID_SysKeyboard, &keyboard, 0) != DI_OK)
		return false;
	if (keyboard->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) != DI_OK)
		return false;
	if (keyboard->SetDataFormat(&c_dfDIKeyboard) != DI_OK)
		return false;
	HRESULT hr = keyboard->Acquire();
	if (hr != DI_OK && hr != S_FALSE)
		return false;

	good = true;
	return true;
}

bool UserInput::updateKeyboard()
{
	good = false;
	if (keyboard->GetDeviceState(256, (void *)&keyState) != DI_OK)
		return false;
	good = true;
	return true;
}


struct JOYDATA
{
	IDirectInput ** pDI;
	IDirectInputDevice2 ** joystick;
};

static BOOL CALLBACK EnumJoysticksCallback(LPCDIDEVICEINSTANCE pInst, LPVOID lpvContext)
{
	IDirectInputDevice * pDID = 0;
	IDirectInput ** pDI = ((JOYDATA *)lpvContext)->pDI;
	IDirectInputDevice2 ** joystick = ((JOYDATA *)lpvContext)->joystick;

	if ((*pDI)->CreateDevice(pInst->guidInstance, &pDID, 0) != DI_OK)
		return DIENUM_CONTINUE;

	if (pDID->QueryInterface(IID_IDirectInputDevice2, (void **)joystick) != S_OK)
	{
		pDID->Release();
		return DIENUM_CONTINUE;
	}

	pDID->Release();
	return DIENUM_STOP;
}

bool UserInput::initJoystick(const HWND hwnd, const int minRange, const int maxRange)
{
	good = false;

	JOYDATA jd = {&pDI, &joystick};
	if (pDI->EnumDevices(DIDEVTYPE_JOYSTICK, EnumJoysticksCallback, (void *)&jd, DIEDFL_ATTACHEDONLY) != DI_OK)
		return false;
	if (!joystick)
		return false;

	if (joystick->SetDataFormat(&c_dfDIJoystick) != DI_OK)
		return false;
	if (joystick->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE) != DI_OK)
		return false;

	DIPROPRANGE diprg; 
	diprg.diph.dwSize = sizeof DIPROPRANGE;
	diprg.diph.dwHeaderSize = sizeof DIPROPHEADER;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = minRange;
	diprg.lMax = maxRange;

	diprg.diph.dwObj = DIJOFS_X;
	HRESULT hr = joystick->SetProperty(DIPROP_RANGE, &diprg.diph);
	if (hr != DI_OK && hr != DI_PROPNOEFFECT)
		return false;

	diprg.diph.dwObj = DIJOFS_Y;
	hr = joystick->SetProperty(DIPROP_RANGE, &diprg.diph);
	if (hr != DI_OK && hr != DI_PROPNOEFFECT)
		return false;

	if (joystick->Acquire() != DI_OK)
		return false;

	good = true;
	return true;
}

bool UserInput::updateJoystick()
{
	good = false;

	if (joystick->Poll() != DI_OK)
		return false;
	if (joystick->GetDeviceState(sizeof DIJOYSTATE, (void *)&joyState) != DI_OK)
		return false;

	good = true;
	return true;
}
