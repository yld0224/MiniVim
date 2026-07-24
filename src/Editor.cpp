#include "Editor.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace sjtu {
namespace {

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char character) {
        return std::isspace(character) != 0;
    };

    const auto first =
        std::find_if_not(value.begin(), value.end(), isSpace);
    const auto last =
        std::find_if_not(value.rbegin(), value.rend(), isSpace).base();
    if (first >= last) {
        return {};
    }
    return {first, last};
}

bool isPrintable(unsigned char value) {
    return value >= 0x20U && value < 0x7FU;
}

} // namespace

Editor::Editor(const std::filesystem::path& path)
    : buffer_(path), terminal_() {}

void Editor::run() {
    while (running_) {
        refreshScreen();
        processKey(terminal_.readKey());
    }
    terminal_.clearScreen();
}

bool Editor::isRunning() const noexcept {
    return running_;
}

void Editor::refreshScreen() {
    window_.resize(terminal_.screenSize());
    window_.ensureCursorVisible(buffer_);

    const auto pendingKeys = normalParser_.pendingDisplay();
    const RenderState state{
        mode_,
        commandLine_,
        message_,
        pendingKeys,
    };
    terminal_.writeOutput(renderer_.render(buffer_, window_, state));
}

void Editor::processKey(KeyEvent key) {
    // Retain the original viewer's emergency exit in every mode.
    if (key.isControl('q')) {
        running_ = false;
        return;
    }

    if (mode_ == Mode::CommandLine) {
        handleCommandLine(key);
        return;
    }

    if (mode_ == Mode::Insert) {
        if (key.code == KeyCode::Escape) {
            mode_ = Mode::Normal;
        }
        return;
    }

    message_.clear();
    execute(normalParser_.feed(key));
}

void Editor::execute(const EditorAction& action) {
    switch (action.kind) {
    case ActionKind::None:
        return;
    case ActionKind::Move:
        window_.applyMotion(buffer_, action.motion, action.count);
        return;
    case ActionKind::EnterCommandLine:
        mode_ = Mode::CommandLine;
        commandLine_.clear();
        message_.clear();
        return;
    case ActionKind::Quit:
        running_ = false;
        return;
    }
}

void Editor::handleCommandLine(KeyEvent key) {
    if (key.code == KeyCode::Escape) {
        leaveCommandLine();
        return;
    }
    if (key.code == KeyCode::Enter) {
        executeCommandLine();
        return;
    }
    if (key.code == KeyCode::Backspace ||
        key.code == KeyCode::Delete) {
        if (!commandLine_.empty()) {
            commandLine_.pop_back();
        }
        return;
    }
    if (key.isControl('u')) {
        commandLine_.clear();
        return;
    }
    if (key.code == KeyCode::Character && isPrintable(key.value)) {
        commandLine_.push_back(static_cast<char>(key.value));
    }
}

void Editor::executeCommandLine() {
    const auto command = trim(commandLine_);
    leaveCommandLine();

    if (command.empty()) {
        return;
    }
    if (command == "q" || command == "quit" ||
        command == "q!" || command == "quit!") {
        running_ = false;
        return;
    }
    if (command == "w" || command == "write" ||
        command == "wq" || command == "x") {
        message_ = "viewer is read-only";
        return;
    }
    if (command == "help") {
        message_ =
            "viewer: hjkl  0 ^ $  gg G  Ctrl-U/D/B/F  :q";
        return;
    }
    message_ = "Not an editor command: " + command;
}

void Editor::leaveCommandLine() {
    mode_ = Mode::Normal;
    commandLine_.clear();
}

} // namespace sjtu
