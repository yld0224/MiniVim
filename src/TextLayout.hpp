#ifndef MINIVIM_TEXT_LAYOUT_HPP
#define MINIVIM_TEXT_LAYOUT_HPP

#include <string_view>
#include <string>

namespace sjtu::text {

inline constexpr size_t tabStop = 4;

inline size_t NextScreenColumn(std::size_t column, unsigned char value) {
    if (value == '\t') { return column + (tabStop - (column % tabStop)); }
    if (value < 0x20U || value == 0x7FU) { return column + 2; }
    return column + 1;
}

inline std::size_t LastColumn(const std::string& line) {
    return line.empty() ? 0 : line.size() - 1;
}


inline std::size_t RenderColumnToBufferColumn(std::string_view line, std::size_t render_column) {
    if (line.empty()) {
        return 0;
    }

    size_t current = 0;
    for (size_t index = 0; index < line.size(); ++index) {
        auto next = NextScreenColumn(current, static_cast<unsigned char>(line[index]));
        if (render_column < next) {
            return index;
        }
        current = next;
    }
    return line.size() - 1;
}

inline size_t BufferColumnToRenderColumn(std::string_view line, size_t buffer_column) {
    size_t column = 0;
    for (size_t index = 0; index < buffer_column; ++index) {
        column = NextScreenColumn(column, static_cast<unsigned char>(line[index]));
    }
    return column;
}
} 

#endif // MINIVIM_TEXT_LAYOUT_HPP
