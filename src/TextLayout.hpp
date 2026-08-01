#ifndef MINIVIM_TEXT_LAYOUT_HPP
#define MINIVIM_TEXT_LAYOUT_HPP

#include <string>
#include <string_view>

namespace sjtu::text {

inline constexpr std::size_t tabStop = 4;

inline std::size_t nextScreenColumn(std::size_t column, unsigned char value) noexcept {
    if (value == '\t') { return column + (tabStop - (column % tabStop)); }
    if (value < 0x20U || value == 0x7FU) { return column + 2; }
    return column + 1;
}

inline std::size_t screenColumn(std::string_view line, std::size_t bufferColumn) noexcept {
    auto limit = std::min(bufferColumn, line.size());
    std::size_t column = 0;
    for (std::size_t index = 0; index < limit; ++index) {
        column = nextScreenColumn(column, static_cast<unsigned char>(line[index]));
    }
    return column;
}

// Return the character whose displayed cell contains targetColumn. A Normal
// mode cursor always points at a character, except that an empty line uses 0.
inline std::size_t bufferColumn(std::string_view line, std::size_t targetColumn) noexcept {
    if (line.empty()) {
        return 0;
    }

    std::size_t current = 0;
    for (std::size_t index = 0; index < line.size(); ++index) {
        const auto next = nextScreenColumn(current, static_cast<unsigned char>(line[index]));
        if (targetColumn < next) {
            return index;
        }
        current = next;
    }
    return line.size() - 1;
}

inline std::string expandForDisplay(std::string_view line) {
    std::string rendered;
    rendered.reserve(line.size());

    std::size_t column = 0;
    for (const char rawValue : line) {
        const auto value = static_cast<unsigned char>(rawValue);
        if (value == '\t') {
            const auto next = nextScreenColumn(column, value);
            rendered.append(next - column, ' ');
            column = next;
        } else if (value < 0x20U || value == 0x7FU) {
            rendered.push_back('^');
            rendered.push_back(
                value == 0x7FU ? '?' : static_cast<char>(value + 0x40U));
            column += 2;
        } else {
            rendered.push_back(rawValue);
            ++column;
        }
    }

    return rendered;
}

} // namespace sjtu::text

#endif // MINIVIM_TEXT_LAYOUT_HPP
