#ifndef MINIVIM_TYPES_HPP
#define MINIVIM_TYPES_HPP

#include <cstddef>

namespace sjtu {

struct Position {
    std::size_t row_{0};
    std::size_t column_{0};
};

struct ScreenSize {
    std::size_t rows_{0};
    std::size_t columns_{0};
};

struct Viewport {
    std::size_t top_{0};
    std::size_t left_{0};
    std::size_t rows_{1};
    std::size_t columns_{1};
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
};

} // namespace sjtu

#endif // MINIVIM_TYPES_HPP
