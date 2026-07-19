/*** includes ***/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string>
#include <termios.h>
#include <unistd.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/
class editorConfig {
public:
	int screenrows;
	int screencols;
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

char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) {
			die("read");
		}
	}
	return c;
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

	if (buf[0] != '\x1b' || buf[1] != ']') {return -1;}
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

/*** input ***/
void editorProcessKeypress() {
	char c = editorReadKey();
	switch (c){
		case CTRL_KEY('q'): {
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		}
	}
}

/*** output ***/
void editorDrawRows(std::string& str){
	for (int y = 0; y < E.screenrows; ++y) {
		str += "~";
		
		if (y < E.screenrows - 1) {
			str += "\r\n";
		}
	}
}

void editorRefreshScreen(){
	std::string str;
	str += "\x1b[2J";
	str += "\x1b[H";

	editorDrawRows(str);

	str += "\x1b[H";
	write(STDOUT_FILENO, str.data(), str.size());
}



/*** init ***/
void initEditor() {
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
		die("getWindowSize");
	}
}

int main(){
	enableRawMode();
	initEditor();

	while (true){
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
