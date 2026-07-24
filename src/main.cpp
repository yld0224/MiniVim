#include "Editor.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "MiniVim: too many arguments\n";
        return 1;
    }

    const std::filesystem::path path = argc == 2 ? argv[1] : "";
    try {
        sjtu::Editor editor(path);
        editor.run();
    } catch (const std::exception& error) {
        std::cerr << "MiniVim: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
