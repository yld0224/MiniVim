#include "Editor.hpp"

#include <algorithm>
#include <cctype>
#include <exception>

namespace sjtu {

namespace {

std::string Trim(std::string value) {
    auto IsSpace = [](unsigned char character) { return std::isspace(character) != 0; };

    auto first = std::find_if_not(value.begin(), value.end(), IsSpace);
    auto last = std::find_if_not(value.rbegin(), value.rend(), IsSpace).base();
    if (first >= last) {return {};}
    return {first, last};
}

bool IsPrintable(unsigned char value) { return value >= 0x20U && value < 0x7FU; }

} // namespace

Editor::Editor(const std::filesystem::path& path) : buffer_(path), terminal_() {}

void Editor::Run() {
    while (running_) {
        RefreshScreen();
        ProcessKey(terminal_.ReadKey());
    }
    terminal_.ClearScreen();
}

bool Editor::IsRunning() const noexcept { return running_; }

void Editor::RefreshScreen() {
    window_.Resize(terminal_.GetScreenSize());
    window_.EnsureCursorVisible(buffer_);

    RenderState state{mode_, command_, message_};
    terminal_.WriteOutput(renderer_.Render(buffer_, window_, state));
}

void Editor::ProcessKey(KeyEvent key) {
    if (key.IsControl('q')) {
        running_ = false;
        return;
    }

    if (mode_ == Mode::CommandLine) {
        HandleCommandLine(key);
        return;
    }

    if (mode_ == Mode::Insert) {
        HandleInsert(key);
        return;
    }

    message_.clear();
    Execute(normal_parser_.Feed(key));
}

void Editor::Execute(const EditorAction& action) {
    switch (action.kind_) {
    case ActionKind::None:
        return;
    case ActionKind::Move:
        window_.ApplyMotion(buffer_, action.motion_.value());
        return;
    case ActionKind::InsertBefore:
        EnterInsert(window_.GetCursor());
        return;
    case ActionKind::InsertAfter: {
        auto cursor = window_.GetCursor();
        if (!buffer_.GetLineAt(cursor.row_).empty()) {
            ++cursor.column_;
        }
        EnterInsert(cursor);
        return;
    }
    case ActionKind::EnterCommandLine:
        mode_ = Mode::CommandLine;
        command_.clear();
        message_.clear();
        return;
    case ActionKind::Quit:
        running_ = false;
        return;
    }
}


void Editor::HandleInsert(KeyEvent key) {
    if (key.code_ == KeyCode::Escape) {
        LeaveInsert();
        return;
    }

    auto cursor = window_.GetCursor();
    if (key.code_ == KeyCode::Enter) {
        buffer_.SplitLine(cursor.row_, cursor.column_);
        ++cursor.row_;
        cursor.column_ = 0;
        window_.SetCursor(buffer_, cursor, true);
        return;
    }

    if (key.code_ == KeyCode::Backspace) {
        if (cursor.column_ > 0) {
            buffer_.EraseCharacter(cursor.row_, cursor.column_ - 1);
            --cursor.column_;
        } else if (cursor.row_ > 0) {
            auto previous_length = buffer_.GetLineAt(cursor.row_ - 1).size();
            buffer_.JoinLine(cursor.row_ - 1);
            --cursor.row_;
            cursor.column_ = previous_length;
        }
        window_.SetCursor(buffer_, cursor, true);
        return;
    }

    if (key.code_ == KeyCode::Character && (IsPrintable(key.value_) || key.value_ == '\t')) {
        buffer_.InsertCharacter( cursor.row_, cursor.column_, static_cast<char>(key.value_));
        ++cursor.column_;
        window_.SetCursor(buffer_, cursor, true);
    }
}
void Editor::EnterInsert(Position position) {
    mode_ = Mode::Insert;
    window_.SetCursor(buffer_, position, true);
    message_.clear();
}

void Editor::LeaveInsert() {
    auto cursor = window_.GetCursor();
    if (cursor.column_ > 0) {
        --cursor.column_;
    }
    window_.SetCursor(buffer_, cursor, false);
    mode_ = Mode::Normal;
}



void Editor::HandleCommandLine(KeyEvent key) {
    if (key.code_ == KeyCode::Escape) {
        LeaveCommandLine();
        return;
    }
    if (key.code_ == KeyCode::Enter) {
        ExecuteCommandLine();
        return;
    }
    if (key.code_ == KeyCode::Backspace || key.code_ == KeyCode::Delete) {
        if (!command_.empty()) {
            command_.pop_back();
        }
        return;
    }
    if (key.code_ == KeyCode::Character && IsPrintable(key.value_)) {
        command_.push_back(static_cast<char>(key.value_));
    }
}

void Editor::ExecuteCommandLine() {
    auto command = Trim(command_);
    LeaveCommandLine();

    if (command.empty()) { return; }

    auto separator = command.find_first_of(" \t");
    auto name = command.substr(0, separator);
    auto argument = separator == std::string::npos ? std::string{} : Trim(command.substr(separator + 1));

    if (name == "q" && argument.empty()) {
        if (buffer_.IsModified()) {
            message_ = "No write since last change (add ! to override)";
            return;
        }
        running_ = false;
        return;
    }
    if ((name == "q!" || name == "quit!") && argument.empty()) {
        running_ = false;
        return;
    }
    if (name == "w" ) {
        SaveBuffer(argument);
        return;
    }
    if (name == "wq") {
        if (SaveBuffer(argument)) {
            running_ = false;
        }
        return;
    }
    
    message_ = "Not an editor command: " + command;
}

void Editor::LeaveCommandLine() {
    mode_ = Mode::Normal;
    command_.clear();
}


bool Editor::SaveBuffer(const std::filesystem::path& path) {
    try {
        if (path.empty()) {
            buffer_.Save();
        } else {
            buffer_.SaveAs(path);
        }
    } catch (const std::exception& error) {
        message_ = error.what();
        return false;
    }

    auto lines = buffer_.GetLineCount();
    message_ = "\"" + buffer_.GetDisplayName() + "\" " + std::to_string(lines) + (lines == 1 ? " line written" : " lines written");
    return true;
}

} // namespace sjtu
