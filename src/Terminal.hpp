#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <system_error>

namespace sjtu{

class Terminal{
private:
    termios ori_termios;

public:

    Terminal() {

        if (tcgetattr(STDIN_FILENO, &ori_termios) == -1){
		    throw std::system_error(errno, std::generic_category(), "tcgetattr");
	    }
	    termios raw = ori_termios;
	    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	    raw.c_oflag &= ~(OPOST);
	    raw.c_cflag |= (CS8);
	    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	    raw.c_cc[VMIN] = 0;
	    raw.c_cc[VTIME] = 1;

	    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1){
		    throw std::system_error(errno, std::generic_category(), "tcsetattr");
	    }
    }

    ~Terminal() noexcept {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &ori_termios);
	}
    
};
}

#endif //TERMINAL_HPP