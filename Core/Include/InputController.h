#pragma once
#include <cstdint>

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

private:
	static InputController* instance;

	// TODO: This can be packed
	bool pressedKeys[0xFF];

};
