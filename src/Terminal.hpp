#ifndef MINIVIM_TERMINAL_HPP
#define MINIVIM_TERMINAL_HPP

#include "Key.hpp"
#include "Types.hpp"

#include <optional>
#include <string_view>
#include <termios.h>

namespace sjtu {

// Owns the terminal's raw-mode lifetime. No editor state is allowed in this
// platform boundary, which keeps the core logic testable without a TTY.
class Terminal {
public:
    Terminal();
    ~Terminal() noexcept;

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal(Terminal&&) = delete;
    Terminal& operator=(Terminal&&) = delete;

    [[nodiscard]] KeyEvent readKey();
    [[nodiscard]] ScreenSize screenSize();
    void writeOutput(std::string_view output);
    void clearScreen();

private:
    [[nodiscard]] unsigned char readByte();
    [[nodiscard]] std::optional<unsigned char> tryReadByte();
    [[nodiscard]] ScreenSize queryCursorPosition();

    termios original_{};
    bool rawModeEnabled_{false};
};

} // namespace sjtu

#endif // MINIVIM_TERMINAL_HPP
