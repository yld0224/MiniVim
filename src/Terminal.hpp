#ifndef MINIVIM_TERMINAL_HPP
#define MINIVIM_TERMINAL_HPP

#include "Key.hpp"
#include "Types.hpp"

#include <optional>
#include <string_view>
#include <termios.h>

namespace sjtu {

class Terminal {
public:
    Terminal();
    ~Terminal() noexcept;

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal(Terminal&&) = delete;
    Terminal& operator=(Terminal&&) = delete;

    KeyEvent ReadKey();
    ScreenSize GetScreenSize();
    void WriteOutput(std::string_view output);
    void ClearScreen();

private:
    unsigned char ReadByte();
    std::optional<unsigned char> TryReadByte();
    ScreenSize QueryCursorPosition();

    termios original_{};
    bool raw_mode_enabled_{false};
};

} // namespace sjtu

#endif // MINIVIM_TERMINAL_HPP
