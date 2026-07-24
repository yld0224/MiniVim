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
    [[nodiscard]] bool isRunning() const noexcept;

private:
    void refreshScreen();
    void processKey(KeyEvent key);
    void execute(const EditorAction& action);
    void handleCommandLine(KeyEvent key);
    void executeCommandLine();
    void leaveCommandLine();

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
