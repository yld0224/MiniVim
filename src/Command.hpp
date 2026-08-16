#ifndef MINIVIM_COMMAND_HPP
#define MINIVIM_COMMAND_HPP

#include <optional>

#include "Key.hpp"
#include "Types.hpp"


namespace sjtu {

enum class ActionKind {
    None,
    Move,
    InsertBefore,
    InsertAfter,
    EnterCommandLine,
    Quit,
};

struct EditorAction {
    ActionKind kind_{ActionKind::None};
    std::optional<Motion> motion_{};
};


class NormalModeParser {

public:
    EditorAction Feed(KeyEvent key);

private:
    EditorAction GenerateMotion(Motion motion);
    EditorAction GenerateCommand(ActionKind kind);

};

} // namespace sjtu

#endif // MINIVIM_COMMAND_HPP
