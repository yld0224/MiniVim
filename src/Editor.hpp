#ifndef MINIVIM_EDITOR_HPP
#define MINIVIM_EDITOR_HPP

#include <filesystem>
#include <string>

#include "Buffer.hpp"
#include "Command.hpp"
#include "Renderer.hpp"
#include "Terminal.hpp"
#include "Types.hpp"
#include "Window.hpp"


namespace sjtu {

class Editor {

public:
    explicit Editor(const std::filesystem::path& path = {});

    void Run();
    bool IsRunning() const noexcept;

private:

    void RefreshScreen();

    void ProcessKey(KeyEvent key);

    void Execute(const EditorAction& action);

    void EnterInsert(Position position);
    void LeaveInsert();
    void HandleInsert(KeyEvent key);

    void HandleCommandLine(KeyEvent key);
    void ExecuteCommandLine();
    void LeaveCommandLine();

    bool SaveBuffer(const std::filesystem::path& path = {});

    Buffer buffer_;
    Terminal terminal_;
    Window window_;
    Renderer renderer_;
    NormalModeParser normal_parser_;

    Mode mode_{Mode::Normal};
    std::string command_;
    std::string message_;
    bool running_{true};
};

} // namespace sjtu

#endif // MINIVIM_EDITOR_HPP
