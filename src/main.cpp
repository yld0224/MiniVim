#include "Editor.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    std::string filename;
    if (argc >= 2) {
        filename = argv[1];
    }
    try {
        sjtu::Editor editor(filename);
        while (editor.isRunning()) {
            editor.refreshScreen();
            editor.processKeypress();
        }
    } catch (const std::exception error) {
        std::cerr << "MiniVim: " << error.what() << '\n';
        return 1;
    }

    return 0;
}