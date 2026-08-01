#include "Editor.hpp"

#include <algorithm>
#include <cctype>
#include <exception>

namespace sjtu {
namespace {

std::string trim(std::string value) {
    auto isSpace = [](unsigned char character) {
        return std::isspace(character) != 0;
    };

    auto first = std::find_if_not(value.begin(), value.end(), isSpace);
    auto last = std::find_if_not(value.rbegin(), value.rend(), isSpace).base();
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

    auto pendingKeys = normalParser_.pendingDisplay();
    RenderState state{mode_, commandLine_, message_, pendingKeys,};
    terminal_.writeOutput(renderer_.render(buffer_, window_, state));
}

void Editor::processKey(KeyEvent key) {
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
    auto count = std::max<std::size_t>(action.count.value_or(1), 1);

    switch (action.kind) {
    case ActionKind::None:
        return;
    case ActionKind::Move:
        window_.applyMotion(buffer_, action.motion.value(), action.count);
        return;
    case ActionKind::InsertBefore:
        enterInsert(window_.cursor());
        return;
    case ActionKind::InsertAfter: {
        auto cursor = window_.cursor();
        if (!buffer_.line(cursor.row).empty()) {
            ++cursor.column;
        }
        enterInsert(cursor);
        return;
    }
    case ActionKind::InsertAtFirstNonBlank:
        window_.applyMotion(buffer_, Motion::FirstNonBlank);
        enterInsert(window_.cursor());
        return;
    case ActionKind::InsertAtLineEnd: {
        auto cursor = window_.cursor();
        cursor.column = buffer_.line(cursor.row).size();
        enterInsert(cursor);
        return;
    }
    case ActionKind::OpenLineBelow:
        openLineBelow();
        return;
    case ActionKind::OpenLineAbove:
        openLineAbove();
        return;
    case ActionKind::DeleteCharacter:
        deleteCharacters(count);
        return;
    case ActionKind::DeleteLine:
        deleteLines(count);
        return;
    case ActionKind::DeleteToLineEnd:
        deleteToLineEnd();
        return;
    case ActionKind::JoinLines:
        joinLines(std::max<std::size_t>( action.count.value_or(2), 2));
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
            auto previousLength = buffer_.line(cursor.row - 1).size();
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

    if (key.code == KeyCode::Character && (isPrintable(key.value) || key.value == '\t')) {
        buffer_.insertCharacter( cursor.row, cursor.column, static_cast<char>(key.value));
        ++cursor.column;
        window_.setInsertCursor(buffer_, cursor);
    }
}
void Editor::enterInsert(Position position) {
    mode_ = Mode::Insert;
    window_.setInsertCursor(buffer_, position);
    message_.clear();
}

void Editor::leaveInsert() {
    auto cursor = window_.cursor();
    if (cursor.column > 0) {
        --cursor.column;
    }
    window_.setNormalCursor(buffer_, cursor);
    mode_ = Mode::Normal;
}

void Editor::openLineBelow() {
    auto cursor = window_.cursor();
    buffer_.insertLine(cursor.row + 1);
    ++cursor.row;
    cursor.column = 0;
    enterInsert(cursor);
}

void Editor::openLineAbove() {
    auto cursor = window_.cursor();
    buffer_.insertLine(cursor.row);
    cursor.column = 0;
    enterInsert(cursor);
}

void Editor::deleteCharacters(std::size_t count) {
    auto cursor = window_.cursor();
    while (count > 0 && cursor.column < buffer_.line(cursor.row).size()) {
        buffer_.eraseCharacter(cursor.row, cursor.column);
        --count;
    }
    window_.setNormalCursor(buffer_, cursor);
}

void Editor::deleteLines(std::size_t count) {
    auto cursor = window_.cursor();
    buffer_.eraseLines(cursor.row, count);
    window_.setNormalCursor(buffer_, cursor);
}

void Editor::deleteToLineEnd() {
    auto cursor = window_.cursor();
    buffer_.eraseToLineEnd(cursor.row, cursor.column);
    window_.setNormalCursor(buffer_, cursor);
}

void Editor::joinLines(std::size_t lineCount) {
    auto cursor = window_.cursor();
    auto joinColumn = buffer_.line(cursor.row).size();
    bool joinedAny = false;

    for (std::size_t joined = 1; joined < lineCount && cursor.row + 1 < buffer_.lineCount(); ++joined) {
        buffer_.joinWithNextLineSeparated(cursor.row);
        joinedAny = true;
    }

    if (!joinedAny) { return; }

    auto& joinedLine = buffer_.line(cursor.row);
    cursor.column = joinedLine.empty() ? 0 : std::min(joinColumn, joinedLine.size() - 1);
    window_.setNormalCursor(buffer_, cursor);
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
    if (key.code == KeyCode::Backspace || key.code == KeyCode::Delete) {
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
    auto command = trim(commandLine_);
    leaveCommandLine();

    if (command.empty()) { return; }

    auto separator = command.find_first_of(" \t");
    auto name = command.substr(0, separator);
    auto argument = separator == std::string::npos ? std::string{} : trim(command.substr(separator + 1));

    if ((name == "q" || name == "quit") && argument.empty()) {
        if (buffer_.isModified()) {
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
    if (name == "w" || name == "write") {
        saveBuffer(argument);
        return;
    }
    if (name == "wq") {
        if (saveBuffer(argument)) {
            running_ = false;
        }
        return;
    }
    if (name == "x") {
        if ((!buffer_.isModified() && argument.empty()) ||
            saveBuffer(argument)) {
            running_ = false;
        }
        return;
    }
    if (name == "help" && argument.empty()) {
        message_ = "editor: hjkl wbe  i a I A o O  x dd D J  :w  :q";
        return;
    }
    message_ = "Not an editor command: " + command;
}

void Editor::leaveCommandLine() {
    mode_ = Mode::Normal;
    commandLine_.clear();
}


bool Editor::saveBuffer(const std::filesystem::path& path) {
    try {
        if (path.empty()) {
            buffer_.save();
        } else {
            buffer_.saveAs(path);
        }
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
