#ifndef MINIVIM_COMMAND_HPP
#define MINIVIM_COMMAND_HPP

#include "Key.hpp"
#include "Types.hpp"

#include <cstddef>
#include <optional>
#include <string>

namespace sjtu {

enum class ActionKind {
    None,
    Move,
    InsertBefore,
    InsertAfter,
    InsertAtFirstNonBlank,
    InsertAtLineEnd,
    OpenLineBelow,
    OpenLineAbove,
    DeleteCharacter,
    DeleteLine,
    DeleteToLineEnd,
    JoinLines,
    EnterCommandLine,
    Quit,
};

struct EditorAction {
    ActionKind kind{ActionKind::None};
    std::optional<Motion> motion{};
    std::optional<std::size_t> count{};
};


class NormalCommandParser {
public:
    EditorAction feed(KeyEvent key);
    std::string pendingDisplay() const;
    void reset() noexcept;

private:
    EditorAction motion(Motion motion);
    EditorAction command(ActionKind kind);
    void appendDigit(unsigned char digit);

    std::optional<std::size_t> count_;
    std::optional<unsigned char> prefix_;
};

} // namespace sjtu

#endif // MINIVIM_COMMAND_HPP
