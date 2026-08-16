#ifndef MINIVIM_KEY_HPP
#define MINIVIM_KEY_HPP

namespace sjtu {

constexpr unsigned char ControlKey(char key) noexcept {
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
    KeyCode code_{KeyCode::Character};
    unsigned char value_{0};

    static constexpr KeyEvent Character(unsigned char value) noexcept {
        return {KeyCode::Character, value};
    }

    constexpr bool IsCharacter(char expected) const noexcept {
        return code_ == KeyCode::Character && value_ == static_cast<unsigned char>(expected);
    }

    constexpr bool IsControl(char expected) const noexcept {
        return code_ == KeyCode::Character && value_ == ControlKey(expected);
    }
};

} // namespace sjtu

#endif // MINIVIM_KEY_HPP
