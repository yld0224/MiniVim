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
    EnterCommandLine,
    Quit,
};

struct EditorAction {
    ActionKind kind{ActionKind::None};
    Motion motion{Motion::Left};
    std::optional<std::size_t> count{};
};

// Parses Normal-mode key sequences independently from their effects. Future
// operator-pending support can extend this state without coupling keys to
// Buffer mutations.
class NormalCommandParser {
public:
    EditorAction feed(KeyEvent key);
    std::string pendingDisplay() const;
    void reset() noexcept;

private:
    EditorAction motion(Motion motion);
    void appendDigit(unsigned char digit) noexcept;

    std::optional<std::size_t> count_;
    std::optional<unsigned char> prefix_;
};

} // namespace sjtu

#endif // MINIVIM_COMMAND_HPP
