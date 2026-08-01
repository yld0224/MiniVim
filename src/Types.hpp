#ifndef MINIVIM_TYPES_HPP
#define MINIVIM_TYPES_HPP

#include <cstddef>

namespace sjtu {

struct Position {
    std::size_t row{0};
    std::size_t column{0};
};

struct ScreenSize {
    std::size_t rows{0};
    std::size_t columns{0};
};

struct Viewport {
    std::size_t top{0};
    std::size_t left{0};
    std::size_t textRows{1};
    std::size_t columns{1};
};

enum class Mode {
    Normal,
    Insert,
    CommandLine,
};

enum class Motion {
    Left,
    Down,
    Up,
    Right,
    WordForward,
    WordBackward,
    WordEnd,
    LineStart,
    FirstNonBlank,
    LineEnd,
    FileStart,
    FileEnd,
    PageUp,
    PageDown,
    HalfPageUp,
    HalfPageDown,
    WindowTop,
    WindowMiddle,
    WindowBottom,
};

} // namespace sjtu

#endif // MINIVIM_TYPES_HPP
