#include "Command.hpp"
#include <limits>

namespace sjtu {

EditorAction NormalCommandParser::feed(KeyEvent key) {
    if (key.isControl('q')) {
        reset();
        return {ActionKind::Quit, std::nullopt, std::nullopt};
    }

    if (key.code == KeyCode::Escape) {
        reset();
        return {};
    }

    if (prefix_.has_value()) {
        if (*prefix_ == 'g' && key.isCharacter('g')) {
            prefix_.reset();
            return motion(Motion::FileStart);
        }
        if (*prefix_ == 'd' && key.isCharacter('d')) {
            return command(ActionKind::DeleteLine);
        }
        reset();
        return {};
    }

    if (key.code == KeyCode::Character) {
        auto value = key.value;
        bool isDigit = value >= '0' && value <= '9';
        if (isDigit && (value != '0' || count_.has_value())) {
            appendDigit(value);
            return {};
        }

        switch (value) {
        case 'h':
            return motion(Motion::Left);
        case 'j':
            return motion(Motion::Down);
        case 'k':
            return motion(Motion::Up);
        case 'l':
            return motion(Motion::Right);
        case 'w':
            return motion(Motion::WordForward);
        case 'b':
            return motion(Motion::WordBackward);
        case 'e':
            return motion(Motion::WordEnd);
        case '0':
            return motion(Motion::LineStart);
        case '^':
            return motion(Motion::FirstNonBlank);
        case '$':
            return motion(Motion::LineEnd);
        case 'G':
            return motion(Motion::FileEnd);
        case 'H':
            return motion(Motion::WindowTop);
        case 'M':
            return motion(Motion::WindowMiddle);
        case 'L':
            return motion(Motion::WindowBottom);
        case 'i':
            return command(ActionKind::InsertBefore);
        case 'a':
            return command(ActionKind::InsertAfter);
        case 'I':
            return command(ActionKind::InsertAtFirstNonBlank);
        case 'A':
            return command(ActionKind::InsertAtLineEnd);
        case 'o':
            return command(ActionKind::OpenLineBelow);
        case 'O':
            return command(ActionKind::OpenLineAbove);
        case 'x':
            return command(ActionKind::DeleteCharacter);
        case 'D':
            return command(ActionKind::DeleteToLineEnd);
        case 'J':
            return command(ActionKind::JoinLines);
        case 'd':
            prefix_ = value;
            return {};
        case 'g':
            prefix_ = value;
            return {};
        case ':':
            return command(ActionKind::EnterCommandLine);
        default:
            break;
        }

        if (key.isControl('b')) {
            return motion(Motion::PageUp);
        }
        if (key.isControl('f')) {
            return motion(Motion::PageDown);
        }
        if (key.isControl('u')) {
            return motion(Motion::HalfPageUp);
        }
        if (key.isControl('d')) {
            return motion(Motion::HalfPageDown);
        }

        reset();
        return {};
    }

    switch (key.code) {
    case KeyCode::ArrowLeft:
        return motion(Motion::Left);
    case KeyCode::ArrowRight:
        return motion(Motion::Right);
    case KeyCode::ArrowUp:
        return motion(Motion::Up);
    case KeyCode::ArrowDown:
        return motion(Motion::Down);
    case KeyCode::Home:
        return motion(Motion::LineStart);
    case KeyCode::End:
        return motion(Motion::LineEnd);
    case KeyCode::PageUp:
        return motion(Motion::PageUp);
    case KeyCode::PageDown:
        return motion(Motion::PageDown);
    default:
        reset();
        return {};
    }
}

void NormalCommandParser::reset() noexcept {
    count_.reset();
    prefix_.reset();
}

EditorAction NormalCommandParser::motion(Motion requestedMotion) {
    EditorAction action{ActionKind::Move, requestedMotion, count_};
    reset();
    return action;
}

EditorAction NormalCommandParser::command(ActionKind kind) {
    EditorAction action{kind, std::nullopt, count_};
    reset();
    return action;
}


void NormalCommandParser::appendDigit(unsigned char digit) {
    auto numericDigit = static_cast<std::size_t>(digit - '0');
    auto current = count_.value_or(0);
    auto maximum = std::numeric_limits<std::size_t>::max();

    if (current > (maximum - numericDigit) / 10) {
        count_ = maximum;
        return;
    }
    count_ = current * 10 + numericDigit;
}

std::string NormalCommandParser::pendingDisplay() const {
    std::string display;
    if (count_.has_value()) {
        display += std::to_string(*count_);
    }
    if (prefix_.has_value()) {
        display.push_back(static_cast<char>(*prefix_));
    }
    return display;
}

} // namespace sjtu
