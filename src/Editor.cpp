#include "Editor.hpp"

#include <algorithm>
#include <cctype>
#include <exception>

namespace sjtu {
namespace {

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char character) {
        return std::isspace(character) != 0;
    };

    const auto first = std::find_if_not(value.begin(), value.end(), isSpace);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), isSpace).base();
    if (first >= last) {
        return {};
    }
    return {first, last};
}

bool isPrintable(unsigned char value) {
    return value >= 0x20U && value < 0x7FU;
}

} // namespace

Editor::Editor(const std::filesystem::path& path) : buffer_(path), terminal_() {}

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
    const RenderState state{mode_, commandLine_, message_, pendingKeys,};
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
        handleInsert(key);
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
    case ActionKind::EnterInsert:
        mode_ = Mode::Insert;
        window_.setInsertCursor(buffer_, window_.cursor());
        message_.clear();
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

void Editor::handleInsert(KeyEvent key) {
    if (key.code == KeyCode::Escape) {
        leaveInsert();
        return;
    }

    auto cursor = window_.cursor();
    if (key.code == KeyCode::Enter) {
        buffer_.splitLine(cursor.row, cursor.column);
        ++cursor.row;
        cursor.column = 0;
        window_.setInsertCursor(buffer_, cursor);
        return;
    }

    if (key.code == KeyCode::Backspace) {
        if (cursor.column > 0) {
            buffer_.eraseCharacter(cursor.row, cursor.column - 1);
            --cursor.column;
        } else if (cursor.row > 0) {
            const auto previousLength = buffer_.line(cursor.row - 1).size();
            buffer_.joinWithNextLine(cursor.row - 1);
            --cursor.row;
            cursor.column = previousLength;
        }
        window_.setInsertCursor(buffer_, cursor);
        return;
    }

    if (key.code == KeyCode::Delete) {
        if (cursor.column < buffer_.line(cursor.row).size()) {
            buffer_.eraseCharacter(cursor.row, cursor.column);
        } else if (cursor.row + 1 < buffer_.lineCount()) {
            buffer_.joinWithNextLine(cursor.row);
        }
        window_.setInsertCursor(buffer_, cursor);
        return;
    }

    if (key.code == KeyCode::Character &&
        (isPrintable(key.value) || key.value == '\t')) {
        buffer_.insertCharacter(
            cursor.row, cursor.column, static_cast<char>(key.value));
        ++cursor.column;
        window_.setInsertCursor(buffer_, cursor);
    }
}

void Editor::leaveInsert() {
    auto cursor = window_.cursor();
    if (cursor.column > 0) {
        --cursor.column;
    }
    window_.setNormalCursor(buffer_, cursor);
    mode_ = Mode::Normal;
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

    if (command == "q" || command == "quit") {
        if (buffer_.isModified()) {
            message_ = "No write since last change (add ! to override)";
            return;
        }
        running_ = false;
        return;
    }
    if (command == "q!" || command == "quit!") {
        running_ = false;
        return;
    }
    if (command == "w" || command == "write") {
        static_cast<void>(saveBuffer());
        return;
    }
    if (command == "wq") {
        if (saveBuffer()) {
            running_ = false;
        }
        return;
    }
    if (command == "x") {
        if (!buffer_.isModified() || saveBuffer()) {
            running_ = false;
        }
        return;
    }
    if (command == "help") {
        message_ = "editor: i  Esc  hjkl  0 ^ $  gg G  :w  :q";
        return;
    }
    message_ = "Not an editor command: " + command;
}

void Editor::leaveCommandLine() {
    mode_ = Mode::Normal;
    commandLine_.clear();
}

bool Editor::saveBuffer() {
    try {
        buffer_.save();
    } catch (const std::exception& error) {
        message_ = error.what();
        return false;
    }

    const auto lines = buffer_.lineCount();
    message_ = "\"" + buffer_.displayName() + "\" " +
               std::to_string(lines) +
               (lines == 1 ? " line written" : " lines written");
    return true;
}

} // namespace sjtu
