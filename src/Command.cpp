#include "Command.hpp"

namespace sjtu {

EditorAction NormalModeParser::Feed(KeyEvent key) {
    
    if (key.IsControl('q')) {
        return {ActionKind::Quit, std::nullopt};
    }

    if (key.code_ == KeyCode::Escape) {
        return {};
    }


    if (key.code_ == KeyCode::Character) {
        auto value = key.value_;
        switch (value) {
        case 'h':
            return GenerateMotion(Motion::Left);
        case 'j':
            return GenerateMotion(Motion::Down);
        case 'k':
            return GenerateMotion(Motion::Up);
        case 'l':
            return GenerateMotion(Motion::Right);
        case 'i':
            return GenerateCommand(ActionKind::InsertBefore);
        case 'a':
            return GenerateCommand(ActionKind::InsertAfter);
        case ':':
            return GenerateCommand(ActionKind::EnterCommandLine);
        default:
            break;
        }

        return {};
    }

    switch (key.code_) {
    case KeyCode::ArrowLeft:
        return GenerateMotion(Motion::Left);
    case KeyCode::ArrowRight:
        return GenerateMotion(Motion::Right);
    case KeyCode::ArrowUp:
        return GenerateMotion(Motion::Up);
    case KeyCode::ArrowDown:
        return GenerateMotion(Motion::Down);
    default:
        return {};
    }

    return {};
}


EditorAction NormalModeParser::GenerateMotion(Motion motion) {
    return {ActionKind::Move, motion};
}

EditorAction NormalModeParser::GenerateCommand(ActionKind kind) {
    return {kind, std::nullopt};
}

} // namespace sjtu
