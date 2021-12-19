#ifndef __INPUT_MANAGER_HPP__
#define __INPUT_MANAGER_HPP__

#include <algorithm>
#include <unordered_map>

struct MouseState {
	int dx;
	int x;
	int dy;
	int y;
	float yOff;
	bool moved;
};

class InputManager{
	public:
		static InputManager *Instance(){
			if(instance == nullptr){
				instance = new InputManager();
			}
			return instance;
		}

		static void destroy() {
			delete InputManager::Instance();
		}

		inline void setMouseState(int x, int y){
			if(x != mouseState.x || y != mouseState.y)
				mouseState.moved = true;
			else
				mouseState.moved = false;
			mouseState.dx = x - mouseState.x;
			mouseState.dy = y - mouseState.y;
			mouseState.x = x;
			mouseState.y = y;
		}

		inline void setKeyValue(int mb, bool isDown){
			buttons[mb] = isDown;
		}

		inline bool isKeyDown(int mb){
			if(buttons.find(mb) == buttons.end()) return false;
			else return buttons[mb];
		}

		inline MouseState getMouseState() const {
			return mouseState;
		}

		inline void scrollState(float yOff){
			mouseState.yOff = yOff;
		}
		inline float getScrollState(){
			return mouseState.yOff;
		}
	private:

		InputManager() {
			mouseState.dx = 0;
			mouseState.dy = 0;
			mouseState.x = 0;
			mouseState.y = 0;
			mouseState.yOff = 0;
			mouseState.moved = false;
		};

		static InputManager* instance;
		std::unordered_map<int, bool> buttons;
		MouseState mouseState;

};
typedef InputManager _InputManager;

#endif
