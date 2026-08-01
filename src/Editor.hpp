#ifndef MINIVIM_EDITOR_HPP
#define MINIVIM_EDITOR_HPP

#include "Buffer.hpp"
#include "Command.hpp"
#include "Renderer.hpp"
#include "Terminal.hpp"
#include "Types.hpp"
#include "Window.hpp"

#include <filesystem>
#include <string>

namespace sjtu {

class Editor {
public:
    explicit Editor(const std::filesystem::path& path = {});

    void run();
    bool isRunning() const noexcept;

private:
    void refreshScreen();
    void processKey(KeyEvent key);
    void execute(const EditorAction& action);
    void enterInsert(Position position);
    void openLineBelow();
    void openLineAbove();
    void deleteCharacters(std::size_t count);
    void deleteLines(std::size_t count);
    void deleteToLineEnd();
    void joinLines(std::size_t lineCount);
    void handleInsert(KeyEvent key);
    void leaveInsert();
    void handleCommandLine(KeyEvent key);
    void executeCommandLine();
    void leaveCommandLine();
    bool saveBuffer(const std::filesystem::path& path = {});

    Buffer buffer_;
    Terminal terminal_;
    Window window_;
    Renderer renderer_;
    NormalCommandParser normalParser_;

    Mode mode_{Mode::Normal};
    std::string commandLine_;
    std::string message_;
    bool running_{true};
};

} // namespace sjtu

#endif // MINIVIM_EDITOR_HPP
