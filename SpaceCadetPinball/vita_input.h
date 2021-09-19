#ifndef __VITA_INPUT_H__
#define __VITA_INPUT_H__

#include <SDL2/SDL.h>

enum VITA_BUTTONS
{
	TRIANGLE,
	CIRCLE,
	CROSS,
	SQUARE,
	LEFT_SHOULDER,
	RIGHT_SHOULDER,
	D_DOWN,
	D_LEFT,
	D_UP,
	D_RIGHT,
	SELECT,
	START
};

static inline void vita_init_joystick()
{
	int numJoysticks = SDL_NumJoysticks();
	if(numJoysticks >= 1)
		SDL_Joystick *vita_controls = SDL_JoystickOpen(0);
}

#endif // __VITA_INPUT_H__