#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Key.hpp"
#include "Terminal.hpp"
#include "Renderer.hpp"
#include "Buffer.hpp"

namespace sjtu{

class Editor{
private:
   Terminal terminal_;
    Cursor cursor_;
    Buffer buffer_;
    Renderer renderer_;

    std::string filename_;
    bool running_ = true;
public:
    Editor(const std::string& filename) : terminal_(), cursor_(), buffer_(filename), renderer_(buffer_, cursor_) {
        filename_ = filename; 
        running_ = true;
    }

    bool isRunning() {
        return running_;
    }

    void processKeypress() {
	    int c = readKey();
	    switch (c){
		    case CTRL_KEY('q'): {
			    write(STDOUT_FILENO, "\x1b[2J", 4);
			    write(STDOUT_FILENO, "\x1b[H", 3);
			    running_ = false;
                break;
		    }

		    case static_cast<int>(editorKey::HOME_KEY):
			    cursor_.cx_ = 0;
      		    break;
		    case static_cast<int>(editorKey::END_KEY):
			    if (cursor_.cy_ < buffer_.rowSize()){
				    cursor_.cx_ = buffer_.colSize(cursor_.cy_);
			    }
			    break;

		    case static_cast<int>(editorKey::PAGE_UP):
		    case static_cast<int>(editorKey::PAGE_DOWN):{
                View v = renderer_.getView();
                size_t screenrow = renderer_.getScreenRow();
			    if (c == static_cast<int>(editorKey::PAGE_UP)) {
          		    cursor_.cy_ = v.rowoff;
        	    } else if (c == static_cast<int>(editorKey::PAGE_DOWN)) {
          		    cursor_.cy_ = std::min(v.rowoff + screenrow - 1, buffer_.rowSize());
        	    }
			    int times = screenrow;
			    while (times--) {
				    moveCursor(c == static_cast<int>(editorKey::PAGE_UP) ? 
				    static_cast<int>(editorKey::ARROW_UP) : static_cast<int>(editorKey::ARROW_DOWN));
			    }
			    break;
		    }

		    case static_cast<int>(editorKey::ARROW_LEFT):
		    case static_cast<int>(editorKey::ARROW_RIGHT):
		    case static_cast<int>(editorKey::ARROW_UP):
		    case static_cast<int>(editorKey::ARROW_DOWN):{
			    moveCursor(c);
			    break;
	        }
        }
    }

    void refreshScreen() {
        renderer_.refreshScreen();
    }

private:
    int readKey() {
	    int nread;
	    char c;
	    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		    if (nread == -1 && errno != EAGAIN) {
			    throw std::runtime_error("read");
		    }
	    }

	    if (c == '\x1b') {
		    char seq[3];
		    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    	    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		    if (seq[0] == '[') {
			    if (seq[1] >= '0' && seq[1] <= '9') {
				    if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        		    if (seq[2] == '~') {
          			    switch (seq[1]) {
						    case '1': {return static_cast<int>(editorKey::HOME_KEY);}
						    case '3': {return static_cast<int>(editorKey::DEL_KEY);}
						    case '4': {return static_cast<int>(editorKey::END_KEY);}
            			    case '5': {return static_cast<int>(editorKey::PAGE_UP);}
            			    case '6': {return static_cast<int>(editorKey::PAGE_DOWN);}
						    case '7': {return static_cast<int>(editorKey::HOME_KEY);}
						    case '8': {return static_cast<int>(editorKey::END_KEY);}
          			    }			
        		    }
			    } else { 
				    switch (seq[1]) {
					    case 'A': {return static_cast<int>(editorKey::ARROW_UP);}
					    case 'B': {return static_cast<int>(editorKey::ARROW_DOWN);}
					    case 'C': {return static_cast<int>(editorKey::ARROW_RIGHT);}
					    case 'D': {return static_cast<int>(editorKey::ARROW_LEFT);}
					    case 'H': {return static_cast<int>(editorKey::HOME_KEY);}
					    case 'F': {return static_cast<int>(editorKey::END_KEY);}
				    }
			    }
		    } else if (seq[0] == 'O') {
			    switch (seq[1]) {
				    case 'H': {return static_cast<int>(editorKey::HOME_KEY);}
				    case 'F': {return static_cast<int>(editorKey::END_KEY);}
			    }
		    }
		    return '\x1b';
	    } else {
		    return c;
	    }
    }
    
    void moveCursor(int key){
	    std::string* row = buffer_.getRow(cursor_.cy_);
	    switch(key) {
		    case static_cast<int>(editorKey::ARROW_LEFT):{
			    if (cursor_.cx_ != 0) {
                    cursor_.cx_--;
                } else if (cursor_.cy_ > 0) {
				    cursor_.cy_--;
				    cursor_.cx_ = buffer_.colSize(cursor_.cy_);
			    }
                break;
		    }
		    case static_cast<int>(editorKey::ARROW_RIGHT):{
			    if (row && cursor_.cx_ < row->size()){
            	    cursor_.cx_++;
			    } else if (row && cursor_.cx_ == row->size()) {
				    cursor_.cy_++;
				    cursor_.cx_ = 0;
			    }
                break;
		    }
		    case static_cast<int>(editorKey::ARROW_UP):{
			    if (cursor_.cy_ != 0) {
                    cursor_.cy_--;
                }
                break;
		    }
		    case static_cast<int>(editorKey::ARROW_DOWN):{
			    if (cursor_.cy_ < buffer_.rowSize()) {
                    cursor_.cy_++;
                }
                break;
		    }
	    }
	    row = buffer_.getRow(cursor_.cy_);
	    size_t rowlen = row ? row->size() : 0;
	    if (cursor_.cx_ > rowlen) {
		    cursor_.cx_ = rowlen;
	    }
    }
};
}
#endif