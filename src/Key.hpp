#ifndef KEY_HPP
#define KEY_HPP

namespace sjtu{
    
#define CTRL_KEY(k) ((k) & 0x1f)
#define TAB_STOP 4

enum class editorKey{
	ARROW_LEFT = 1000,
  	ARROW_RIGHT,
  	ARROW_UP,
  	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};
}
#endif //KEY_HPP