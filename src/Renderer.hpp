#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <sys/ioctl.h>
#include <unistd.h>

#include "Buffer.hpp"
#include "Key.hpp"

namespace sjtu{

struct Cursor{
    size_t cx_ = 0;
    size_t cy_ = 0;
};
struct View {
    size_t coloff = 0;
    size_t rowoff = 0;
};

class Renderer{
private:
    size_t screenrows = 0;
    size_t screencols = 0;
    View view_;
    Buffer& buffer_;
    Cursor& cursor_;
    size_t rx_;
public:
    Renderer(Buffer& buffer, Cursor& cursor): buffer_(buffer), cursor_(cursor) {
        if (initScreenSize() == -1) {
            throw std::system_error(errno, std::generic_category(), "getscreensize");
        }
        screenrows -= 1;
    }

    ~Renderer() = default;

    int initScreenSize() {
        winsize ws;
	    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
                return -1;
            }
    	    return getCursorPosition(&screenrows, &screencols);
	    } else {
		    screencols = ws.ws_col;
		    screenrows = ws.ws_row;
            return 0;
	    }
    }

    int getCursorPosition(size_t *rows, size_t *cols) {
	    char buf[32];
	    size_t i = 0;

	    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {return -1;}

  	    while (i < sizeof(buf) - 1) {
		    if (read(STDIN_FILENO, &buf[i], 1) != 1) {break;}
		    if (buf[i] == 'R') {break;}
		    ++i;
	    }
	    buf[i] = '\0';

	    if (buf[0] != '\x1b' || buf[1] != '[') {return -1;}
	    if (sscanf(&buf[2], "%ld;%ld", rows, cols) != 2) {return -1;}

  	    return 0;
    }

    View getView() {return view_;}
    size_t getScreenRow() {return screenrows;}
    size_t getScreenCol() {return screencols;}
    

    void editorDrawRows(std::string& str){
	    for (int y = 0; y < screenrows; ++y) {
		    int filerow = y + view_.rowoff;
		    if (filerow >= buffer_.rowSize()) {
			    str += "~";
		    } else {
                std::string row_render = generateRender(*(buffer_.getRow(filerow)));
			    int len = row_render.size() - view_.coloff;
			    if (len < 0) {len = 0;}
			    if (len > screencols) {len = screencols;}
			    if (len > 0) {str += row_render.substr(view_.coloff, len);}
		    }
		    str += "\x1b[K";
		    str += "\r\n";
	    }
    }

    void editorDrawStatusbar(std::string& str){
	    str += "\x1b[7m";
	    int len;
        std::string filename = buffer_.getFilename();
	    str += filename.substr(0, std::min(screencols, filename.size()));
	    len = std::min(screencols, filename.size());
	    for (int i = len; i < screencols; ++i) {
		    str += " ";
	    }
	    str += "\x1b[m";
    }

    void refreshScreen(){
        scroll();

	    std::string str;
	    str += "\x1b[?25l";
	    str += "\x1b[H";

	    editorDrawRows(str);
	    editorDrawStatusbar(str);

	    char buf[32];
  	    int len = snprintf(buf, sizeof(buf), "\x1b[%ld;%ldH", (cursor_.cy_ - view_.rowoff) + 1, (rx_ - view_.coloff) + 1);
  	    str.append(buf, len); 

	    str += "\x1b[?25h";
	    write(STDOUT_FILENO, str.data(), str.size());
    }

    std::string generateRender(const std::string& string) {
        std::string render;
        for (char ch : string) {
            if (ch != '\t') {
                render += ch;
            } else {
                do {
                    render += ' ';
                } while (render.size() % TAB_STOP != 0);
            }
        }
        return render;
    }

    void scroll() {
	    rx_ = 0;
	    if (cursor_.cy_ < buffer_.rowSize()) {
		    rx_ = CxToRx(*(buffer_.getRow(cursor_.cy_)), cursor_.cx_);
	    }
	    if (cursor_.cy_ < view_.rowoff) {
		    view_.rowoff = cursor_.cy_;
	    }
	    if (cursor_.cy_ >= view_.rowoff + screenrows) {
		    view_.rowoff = cursor_.cy_ - screenrows + 1;
	    }
	    if (rx_ < view_.coloff) {
		    view_.coloff = rx_;
	    }
	    if (rx_ >= view_.coloff + screencols) {
		    view_.coloff = rx_ - screencols + 1;
	    }
    }

    size_t CxToRx(const std::string& row, size_t cx) {
        int rx = 0;
  		for (int j = 0; j < cx; j++) {
    		if (row[j] == '\t'){
      			rx += (TAB_STOP - 1) - (rx % TAB_STOP);
			}
    		rx++;
  		}
  		return rx;
    }
};
}
#endif //RENDERER_HPP