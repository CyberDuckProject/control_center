#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <SDL_gamecontroller.h>
#include <stdexcept>

class Controller {
public:
  Controller() { find_controller(); }
  void find_controller() {
    if (working())
      return;

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
      if (SDL_IsGameController(i)) {
        controller = SDL_GameControllerOpen(i);
        if (controller) {
          break;
        }
      }
    }
  }
  float left_x() {
    return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) /
           32767.0f;
  }
  float right_x() {
    return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) /
           32767.0f;
  }
  float left_y() {
    return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) /
           32767.0f;
  }
  float right_y() {
    return SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) /
           32767.0f;
  }
  bool working() {
    return controller && SDL_GameControllerGetAttached(controller);
  }
  ~Controller() {
    if (working())
      SDL_GameControllerClose(controller);
  }

private:
  SDL_GameController *controller = nullptr;
};

#endif