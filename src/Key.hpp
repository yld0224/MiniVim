#ifndef MINIVIM_KEY_HPP
#define MINIVIM_KEY_HPP

namespace sjtu {

constexpr unsigned char controlKey(char key) noexcept {
    return static_cast<unsigned char>(key) & 0x1FU;
}

enum class KeyCode {
    Character,
    Escape,
    Enter,
    Backspace,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    ArrowDown,
};

struct KeyEvent {
    KeyCode code{KeyCode::Character};
    unsigned char value{0};

    static constexpr KeyEvent character(unsigned char value) noexcept {
        return {KeyCode::Character, value};
    }

    constexpr bool isCharacter(char expected) const noexcept {
        return code == KeyCode::Character && value == static_cast<unsigned char>(expected);
    }

    constexpr bool isControl(char expected) const noexcept {
        return code == KeyCode::Character && value == controlKey(expected);
    }
};

} // namespace sjtu

#endif // MINIVIM_KEY_HPP
