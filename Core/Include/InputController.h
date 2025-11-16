#pragma once
#include <cstdint>
#include <windows.h>

class InputController
{
public:
	InputController() = default;

	struct KeyState
	{
		uint8_t bIsPressed : 1;
		uint8_t bWasPressedLastFrame : 1;
	};

	static void Initialize()
	{
		instance = new InputController();
		for (int i = 0; i < _countof(instance->keysState); i++)
		{
			instance->keysState[i] = { .bIsPressed = false, .bWasPressedLastFrame = false };
		}
	}

	static void SetKeyPressed(uint8_t keyIndex, bool pressed = true)
	{
		instance->keysState[keyIndex].bWasPressedLastFrame = instance->keysState[keyIndex].bIsPressed;
		instance->keysState[keyIndex].bIsPressed = pressed;
	}

	static bool IsKeyPressed(uint8_t keyIndex)
	{
		return instance->keysState[keyIndex].bIsPressed;
	}

	static bool IsKeyDown(uint8_t keyIndex)
	{
		return !instance->keysState[keyIndex].bWasPressedLastFrame && instance->keysState[keyIndex].bIsPressed;
	}

	// TODO: when packed, this check should be much more efficient
	static bool HasMovementInput()
	{
		return IsKeyPressed('D') || IsKeyPressed('A') || IsKeyPressed('E') ||
			IsKeyPressed('Q') || IsKeyPressed('S') || IsKeyPressed('W');
	}

	static void UpdateMouseMovement()
	{
		instance->mouseState.UpdateMouseMovement();
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

	static void SetMousePixelAfterOffset(int pixelX, int pixelY)
	{
		instance->mouseState.pixelAfterOffsetX = pixelX;
		instance->mouseState.pixelAfterOffsetY = pixelY;
	}

private:
	static InputController* instance;

	KeyState keysState[0xFF];

	struct Mouse
	{
		int currentPixelX;
		int currentPixelY;
		int pixelAfterOffsetX;
		int pixelAfterOffsetY;
		int offsetX;
		int offsetY;

		void UpdateMouseMovement()
		{
			if ((GetKeyState(VK_RBUTTON) & 0x8000) != 0)
			{
				offsetX = pixelAfterOffsetX - currentPixelX;
				offsetY = pixelAfterOffsetY - currentPixelY;
			}
			else
			{
				offsetX = 0;
				offsetY = 0;
			}

			currentPixelX = pixelAfterOffsetX;
			currentPixelY = pixelAfterOffsetY;
		}
	} mouseState;
};
