/*** includes ***/
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

/*** defines ***/
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
/*** data ***/
class editorRow{
public:
	std::string chars_;
	std::string render_;

	explicit editorRow(const std::string& str) : chars_(str) {
        for (char ch : chars_) {
            if (ch != '\t') {
                render_ += ch;
            } else {
                do {
                    render_ += ' ';
                } while (render_.size() % TAB_STOP != 0);
            }
        }
    }

	size_t CxToRx(int cx){
		int rx = 0;
  		for (int j = 0; j < cx; j++) {
    		if (this->chars_[j] == '\t'){
      			rx += (TAB_STOP - 1) - (rx % TAB_STOP);
			}
    		rx++;
  		}
  		return rx;
	}
};

class editorConfig {
public:
	size_t cx = 0;
	size_t cy = 0;
	size_t rx = 0;
	size_t rowoff = 0;
	size_t coloff = 0;
	int screenrows;
	int screencols;
	std::vector<editorRow> rows{};
	std::string filename;
	termios ori_termios;
};
editorConfig E;

/*** terminal ***/
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}

void disableRawMode(){
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.ori_termios) == -1){
		die("tcsetattr");
	}
}

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.ori_termios) == -1){
		die("tcgetattr");
	}
	atexit(disableRawMode);
	
	termios raw = E.ori_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
		die("tcsetattr");
	}
}

int editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read");
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

int getCursorPosition(int *rows, int *cols) {
	char buf[32];
	uint i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {return -1;}

  	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) {break;}
		if (buf[i] == 'R') {break;}
		++i;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') {return -1;}
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {return -1;}

  	return 0;
}

int getWindowSize(int *rows, int *cols){
	winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {return -1;}
    	return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** file i/o***/
void editorOpen(const std::string& filename) {
	std::ifstream file(filename);
	E.filename = filename;
	if (!file.is_open()) {
		die("open");
	}
	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		E.rows.emplace_back(line);
	}
	if (file.bad()) {die("read file");}
}


/*** input ***/
void editorMoveCursor(int key){
	editorRow *row = (E.cy >= E.rows.size()) ? nullptr : &E.rows[E.cy];
	switch(key) {
		case static_cast<int>(editorKey::ARROW_LEFT):{
			if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
				E.cy--;
				E.cx = E.rows[E.cy].chars_.size();
			}
            break;
		}
		case static_cast<int>(editorKey::ARROW_RIGHT):{
			if (row && E.cx < row->chars_.size()){
            	E.cx++;
			} else if (row && E.cx == row->chars_.size()) {
				E.cy++;
				E.cx = 0;
			}
            break;
		}
		case static_cast<int>(editorKey::ARROW_UP):{
			if (E.cy != 0) {
                E.cy--;
            }
            break;
		}
		case static_cast<int>(editorKey::ARROW_DOWN):{
			if (E.cy < E.rows.size()) {
                E.cy++;
            }
            break;
		}
	}
	row = (E.cy >= E.rows.size()) ? nullptr : &E.rows[E.cy];
	size_t rowlen = row ? row->chars_.size() : 0;
	if (E.cx > rowlen) {
		E.cx = rowlen;
	}
}

void editorProcessKeypress() {
	int c = editorReadKey();
	switch (c){
		case CTRL_KEY('q'): {
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		}

		case static_cast<int>(editorKey::HOME_KEY):
			E.cx = 0;
      		break;
		case static_cast<int>(editorKey::END_KEY):
			if (E.cy < E.rows.size()){
				E.cx = E.rows[E.cy].chars_.size();
			}
			break;

		case static_cast<int>(editorKey::PAGE_UP):
		case static_cast<int>(editorKey::PAGE_DOWN):{
			if (c == static_cast<int>(editorKey::PAGE_UP)) {
          		E.cy = E.rowoff;
        	} else if (c == static_cast<int>(editorKey::PAGE_DOWN)) {
          		E.cy = std::min(E.rowoff + E.screenrows - 1, E.rows.size());
        	}
			int times = E.screenrows;
			while (times--) {
				editorMoveCursor(c == static_cast<int>(editorKey::PAGE_UP) ? 
				static_cast<int>(editorKey::ARROW_UP) : static_cast<int>(editorKey::ARROW_DOWN));
			}
			break;
		}

		case static_cast<int>(editorKey::ARROW_LEFT):
		case static_cast<int>(editorKey::ARROW_RIGHT):
		case static_cast<int>(editorKey::ARROW_UP):
		case static_cast<int>(editorKey::ARROW_DOWN):
			editorMoveCursor(c);
			break;
	}
}

/*** output ***/
void editorScroll() {
	E.rx = 0;
	if (E.cy < E.rows.size()) {
		E.rx = E.rows[E.cy].CxToRx(E.cx);
	}
	if (E.cy < E.rowoff) {
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1;
	}
	if (E.rx < E.coloff) {
		E.coloff = E.rx;
	}
	if (E.rx >= E.coloff + E.screencols) {
		E.coloff = E.rx - E.screencols + 1;
	}
}

void editorDrawRows(std::string& str){
	for (int y = 0; y < E.screenrows; ++y) {
		int filerow = y + E.rowoff;
		if (filerow >= static_cast<int>(E.rows.size())) {
			str += "~";
		} else {
			int len = E.rows[filerow].render_.size() - E.coloff;
			if (len < 0) {len = 0;}
			if (len > E.screencols) {len = E.screencols;}
			if (len > 0) {str += E.rows[filerow].render_.substr(E.coloff, len);}
		}
		str += "\x1b[K";
		str += "\r\n";
	}
}

void editorDrawStatusbar(std::string& str){
	str += "\x1b[7m";
	int len = 0;
	if (E.filename.empty()) {
		str += "[Unnamed]";
		len = 9;
	} else {
		str += E.filename.substr(0, std::min(E.screencols, static_cast<int>(E.filename.size())));
		len = std::min(E.screencols, static_cast<int>(E.filename.size()));
	}
	for (int i = len; i < E.screencols; ++i) {
		str += " ";
	}
	str += "\x1b[m";
}

void editorRefreshScreen(){
	editorScroll();

	std::string str;
	str += "\x1b[?25l";
	str += "\x1b[H";

	editorDrawRows(str);
	editorDrawStatusbar(str);

	char buf[32];
  	int len = snprintf(buf, sizeof(buf), "\x1b[%ld;%ldH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
  	str.append(buf, len); 

	str += "\x1b[?25h";
	write(STDOUT_FILENO, str.data(), str.size());
}



/*** init ***/
void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize");
	}
	E.screenrows -= 1;
}

int main(int argc, char *argv[]){
	enableRawMode();
	initEditor();
	if (argc > 2) {
		std::string msg = "Too many arguments";
		write(STDOUT_FILENO, msg.data(), msg.size());
		exit(1);
	}
	if (argc == 2) {
		editorOpen(argv[1]);
	}

	while (true){
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}