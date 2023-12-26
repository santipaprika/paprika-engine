#pragma once
#include <cstdint>
#include <windows.h>

class InputController
{
public:
	InputController() = default;

	static void Initialize()
	{
		instance = new InputController();
	}

	static void SetKeyPressed(uint8_t keyIndex, bool pressed = true)
	{
		instance->pressedKeys[keyIndex] = pressed;
	}

	static bool IsKeyPressed(uint8_t keyIndex)
	{
		return instance->pressedKeys[keyIndex];
	}

	// TODO: when packed, this check should be much more efficient
	static bool HasMovementInput()
	{
		return IsKeyPressed('D') || IsKeyPressed('A') || IsKeyPressed('E') ||
			IsKeyPressed('Q') || IsKeyPressed('S') || IsKeyPressed('W');
	}

	static void UpdateMouseMovement(int pixelX, int pixelY, DWORD flags)
	{
		if (flags & MK_RBUTTON)
		{
			instance->mouseState.offsetX = pixelX - instance->mouseState.currentPixelX;
			instance->mouseState.offsetY = pixelY - instance->mouseState.currentPixelY;
		}
		else
		{
			instance->mouseState.offsetX = 0;
			instance->mouseState.offsetY = 0;
		}

		instance->mouseState.currentPixelX = pixelX;
		instance->mouseState.currentPixelY = pixelY;
	}

	static bool HasMouseInput()
	{
		return instance->mouseState.offsetX != 0 || instance->mouseState.offsetY != 0;
	}

	static float GetMouseOffsetX()
	{
		return instance->mouseState.offsetX / 720.f;
	}

	static float GetMouseOffsetY()
	{
		return instance->mouseState.offsetY / 720.f;
	}

private:
	static InputController* instance;

	// TODO: This can be packed
	bool pressedKeys[0xFF];

	struct Mouse
	{
		int currentPixelX;
		int currentPixelY;
		int offsetX;
		int offsetY;

	} mouseState;
};
