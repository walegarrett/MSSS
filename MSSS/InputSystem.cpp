// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#include "InputSystem.h"
#include <SDL/SDL.h>
#include <cstring>

bool Keyboard::GetKeyValue(SDL_Scancode keyCode) const
{
	return mCurrState[keyCode] == 1;
}

ButtonState Keyboard::GetKeyState(SDL_Scancode keyCode) const
{
	if (mPrevState[keyCode] == 0)
	{
		if (mCurrState[keyCode] == 0)
		{
			return ENone;
		}
		else
		{
			return EPressed;
		}
	}
	else // Prev state must be 1
	{
		if (mCurrState[keyCode] == 0)
		{
			return EReleased;
		}
		else
		{
			return EHeld;
		}
	}
}

bool Mouse::GetButtonValue(int button) const
{
	return (SDL_BUTTON(button) & mCurrButtons) == 1;
}

ButtonState Mouse::GetButtonState(int button) const
{
	int mask = SDL_BUTTON(button);
	if ((mask & mPrevButtons) == 0)
	{
		if ((mask & mCurrButtons) == 0)
		{
			return ENone;
		}
		else
		{
			return EPressed;
		}
	}
	else
	{
		if ((mask & mCurrButtons) == 0)
		{
			return EReleased;
		}
		else
		{
			return EHeld;
		}
	}
}

bool Controller::GetButtonValue(SDL_GameControllerButton button) const
{
	return mCurrButtons[button] == 1;
}

ButtonState Controller::GetButtonState(SDL_GameControllerButton button) const
{
	if (mPrevButtons[button] == 0)
	{
		if (mCurrButtons[button] == 0)
		{
			return ENone;
		}
		else
		{
			return EPressed;
		}
	}
	else // Prev state must be 1
	{
		if (mCurrButtons[button] == 0)
		{
			return EReleased;
		}
		else
		{
			return EHeld;
		}
	}
}

bool InputSystem::Initialize()
{
	// Keyboard
	// Assign current state pointer
	mDevice.keyboard.mCurrState = SDL_GetKeyboardState(NULL);
	// Clear previous state memory
	memset(mDevice.keyboard.mPrevState, 0, SDL_NUM_SCANCODES);

	// Mouse (just set everything to 0)
	mDevice.mouse.mCurrButtons = 0;
	mDevice.mouse.mPrevButtons = 0;

	// Get the connected controller, if it exists
	mController = SDL_GameControllerOpen(0);
	// Initialize controller state
	mDevice.controller.mIsConnected = (mController != nullptr);
	memset(mDevice.controller.mCurrButtons, 0, SDL_CONTROLLER_BUTTON_MAX);
	memset(mDevice.controller.mPrevButtons, 0, SDL_CONTROLLER_BUTTON_MAX);	

	return true;
}

void InputSystem::Shutdown()
{
}

void InputSystem::PrepareForUpdate()
{
	// Copy current state to previous
	// Keyboard
	memcpy(mDevice.keyboard.mPrevState,
		mDevice.keyboard.mCurrState,
		SDL_NUM_SCANCODES);

	// Mouse
	mDevice.mouse.mPrevButtons = mDevice.mouse.mCurrButtons;
	mDevice.mouse.mIsRelative = false;
	mDevice.mouse.mScrollWheel = Vector2::Zero;

	// Controller
	memcpy(mDevice.controller.mPrevButtons,
		mDevice.controller.mCurrButtons,
		SDL_CONTROLLER_BUTTON_MAX);
}

void InputSystem::Update()
{
	// Mouse
	int x = 0, y = 0;
	if (mDevice.mouse.mIsRelative)
	{
		mDevice.mouse.mCurrButtons = 
			SDL_GetRelativeMouseState(&x, &y);
	}
	else
	{
		mDevice.mouse.mCurrButtons = 
			SDL_GetMouseState(&x, &y);
	}

	mDevice.mouse.mMousePos.x = static_cast<float>(x);
	mDevice.mouse.mMousePos.y = static_cast<float>(y);

	// Controller
	// Buttons
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		mDevice.controller.mCurrButtons[i] = SDL_GameControllerGetButton(mController, SDL_GameControllerButton(i));
	}

	// Triggers
	mDevice.controller.mLeftTrigger = Filter1D(SDL_GameControllerGetAxis(mController, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
	mDevice.controller.mRightTrigger = Filter1D(SDL_GameControllerGetAxis(mController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

	// Sticks
	x = SDL_GameControllerGetAxis(mController, SDL_CONTROLLER_AXIS_LEFTX);
	y = -SDL_GameControllerGetAxis(mController, SDL_CONTROLLER_AXIS_LEFTY);
	//SDL_Log("x = %d : y = %d", x, y);
	mDevice.controller.mLeftStick = Filter2D(x, y);
	//SDL_Log("x = %f : y = %f", mDevice.controller.mLeftStick.x, mDevice.controller.mLeftStick.y);

	x = SDL_GameControllerGetAxis(mController, SDL_CONTROLLER_AXIS_RIGHTX);
	y = -SDL_GameControllerGetAxis(mController, SDL_CONTROLLER_AXIS_RIGHTY);
	//SDL_Log("x = %d : y = %d", x, y);
	mDevice.controller.mRightStick = Filter2D(x, y);
}

void InputSystem::ProcessEvent(SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_MOUSEWHEEL:
		mDevice.mouse.mScrollWheel = Vector2(
			static_cast<float>(event.wheel.x),
			static_cast<float>(event.wheel.y));
		break;
	default:
		break;
	}
}

void InputSystem::SetRelativeMouseMode(bool value)
{
	SDL_bool set = value ? SDL_TRUE : SDL_FALSE;
	SDL_SetRelativeMouseMode(set);

	mDevice.mouse.mIsRelative = value;
}

float InputSystem::Filter1D(int input)
{
	// A value < dead zone is interpreted as 0%
	const int deadZone = 250;
	// A value > max value is interpreted as 100%
	const int maxValue = 30000;

	float retVal = 0.0f;

	// Take absolute value of input
	int absValue = input > 0 ? input : -input;
	// Ignore input within dead zone
	if (absValue > deadZone)
	{
		// Compute fractional value between dead zone and max value
		retVal = static_cast<float>(absValue - deadZone) /
		(maxValue - deadZone);
		// Make sure sign matches original value
		retVal = input > 0 ? retVal : -1.0f * retVal;
		// Clamp between -1.0f and 1.0f
		retVal = Math::Clamp(retVal, -1.0f, 1.0f);
	}

	return retVal;
}

Vector2 InputSystem::Filter2D(int inputX, int inputY)
{
	const float deadZone = 8000.0f;
	const float maxValue = 30000.0f;

	// Make into 2D vector
	Vector2 dir;
	dir.x = static_cast<float>(inputX);
	dir.y = static_cast<float>(inputY);

	float length = dir.Length();
	//SDL_Log("length = %f ", length);
	// If length < deadZone, should be no input
	if (length < deadZone)
	{
		dir = Vector2::Zero;
	}
	else
	{
		// Calculate fractional value between
		// dead zone and max value circles
		float f = (length - deadZone) / (maxValue - deadZone);
		// Clamp f between 0.0f and 1.0f
		f = Math::Clamp(f, 0.0f, 1.0f);		
		// Normalize the vector, and then scale it to the
		// fractional value
		dir *= f / length;
		//SDL_Log("length = %f, x = %f : y = %f", dir.Length(), dir.x, dir.y);
	}

	return dir;
}
