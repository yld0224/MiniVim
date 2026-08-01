#include "Window.hpp"
#include "TextLayout.hpp"

#include <algorithm>
#include <cctype>
#include <limits>

namespace sjtu {
namespace {

enum class WordClass {
    Whitespace,
    Word,
    Punctuation,
};

WordClass wordClass(char rawValue) {
    const auto value = static_cast<unsigned char>(rawValue);
    if (std::isspace(value) != 0) {
        return WordClass::Whitespace;
    }
    if (std::isalnum(value) != 0 || value == '_') {
        return WordClass::Word;
    }
    return WordClass::Punctuation;
}

std::size_t firstNonWhitespace(const std::string& line, std::size_t start) {
    auto column = std::min(start, line.size());
    while (column < line.size() &&
           wordClass(line[column]) == WordClass::Whitespace) {
        ++column;
    }
    return column;
}

} // namespace

void Window::resize(ScreenSize terminalSize) {
    viewport_.columns = std::max<std::size_t>(terminalSize.columns, 1);
    viewport_.textRows = terminalSize.rows > 2 ? terminalSize.rows - 2 : 1;
}

void Window::applyMotion(const Buffer& buffer, Motion motion, std::optional<std::size_t> requestedCount) {
    const auto count = std::max<std::size_t>(requestedCount.value_or(1), 1);

    switch (motion) {
    case Motion::Left:
        moveLeft(buffer, count);
        break;
    case Motion::Right:
        moveRight(buffer, count);
        break;
    case Motion::WordForward:
        moveWordForward(buffer, count);
        break;
    case Motion::WordBackward:
        moveWordBackward(buffer, count);
        break;
    case Motion::WordEnd:
        moveWordEnd(buffer, count);
        break;
    case Motion::Up:
        moveUp(buffer, count);
        break;
    case Motion::Down:
        moveDown(buffer, count);
        break;
    case Motion::LineStart:
        cursor_.column = 0;
        updateDesiredColumn(buffer);
        break;
    case Motion::FirstNonBlank:
        cursor_.column = firstNonBlank(buffer.line(cursor_.row));
        updateDesiredColumn(buffer);
        break;
    case Motion::LineEnd:
        if (count > 1) {
            moveDown(buffer, count - 1);
        }
        cursor_.column = lastColumn(buffer.line(cursor_.row));
        updateDesiredColumn(buffer);
        break;
    case Motion::FileStart: {
        const auto target = requestedCount.has_value() ? count - 1 : 0;
        cursor_.row = std::min(target, buffer.lineCount() - 1);
        moveToLineEdge(buffer, true);
        break;
    }
    case Motion::FileEnd: {
        const auto target = requestedCount.has_value() ? count - 1 : buffer.lineCount() - 1;
        cursor_.row = std::min(target, buffer.lineCount() - 1);
        moveToLineEdge(buffer, true);
        break;
    }
    case Motion::PageUp:
        moveUp(buffer, scaledStep(viewport_.textRows, count));
        break;
    case Motion::PageDown:
        moveDown(buffer, scaledStep(viewport_.textRows, count));
        break;
    case Motion::HalfPageUp:
        moveUp(buffer, scaledStep(std::max<std::size_t>(viewport_.textRows / 2, 1),count));
        break;
    case Motion::HalfPageDown:
        moveDown(buffer,scaledStep(std::max<std::size_t>(viewport_.textRows / 2, 1), count));
        break;
    case Motion::WindowTop: {
        const auto offset = requestedCount.has_value() ? count - 1 : 0;
        cursor_.row = std::min(viewport_.top + offset, buffer.lineCount() - 1);
        moveToLineEdge(buffer, true);
        break;
    }
    case Motion::WindowMiddle: {
        const auto visibleBottom = std::min(viewport_.top + viewport_.textRows - 1, buffer.lineCount() - 1);
        cursor_.row = viewport_.top + (visibleBottom - viewport_.top) / 2;
        moveToLineEdge(buffer, true);
        break;
    }
    case Motion::WindowBottom: {
        const auto visibleBottom = std::min(viewport_.top + viewport_.textRows - 1, buffer.lineCount() - 1);
        const auto offset = requestedCount.has_value() ? count - 1 : 0;
        cursor_.row = offset > visibleBottom - viewport_.top ? viewport_.top : visibleBottom - offset;
        moveToLineEdge(buffer, true);
        break;
    }
    }

    normalize(buffer);
    ensureCursorVisible(buffer);
}

void Window::ensureCursorVisible(const Buffer& buffer) {

    if (cursor_.row < viewport_.top) {
        viewport_.top = cursor_.row;
    } else if (cursor_.row >= viewport_.top + viewport_.textRows) {
        viewport_.top = cursor_.row - viewport_.textRows + 1;
    }

    const auto screenColumn = cursorScreenColumn(buffer);
    if (screenColumn < viewport_.left) {
        viewport_.left = screenColumn;
    } else if (screenColumn >= viewport_.left + viewport_.columns) {
        viewport_.left = screenColumn - viewport_.columns + 1;
    }
}

void Window::setNormalCursor(const Buffer& buffer, Position position) {
    setCursor(buffer, position, false);
}

void Window::setInsertCursor(const Buffer& buffer, Position position) {
    setCursor(buffer, position, true);
}

const Position& Window::cursor() const noexcept {
    return cursor_;
}

const Viewport& Window::viewport() const noexcept {
    return viewport_;
}

std::size_t Window::cursorScreenColumn(const Buffer& buffer) const {
    return text::screenColumn(buffer.line(cursor_.row), cursor_.column);
}

std::size_t Window::lastColumn(const std::string& line) noexcept {
    return line.empty() ? 0 : line.size() - 1;
}

std::size_t Window::firstNonBlank(const std::string& line) noexcept {
    const auto position = line.find_first_not_of(" \t");
    return position == std::string::npos ? 0 : position;
}

std::size_t Window::scaledStep(std::size_t step, std::size_t count) noexcept {
    if (step == 0 || count == 0) {
        return 0;
    }
    if (count > std::numeric_limits<std::size_t>::max() / step) {
        return std::numeric_limits<std::size_t>::max();
    }
    return step * count;
}

void Window::normalize(const Buffer& buffer) {
    cursor_.row = std::min(cursor_.row, buffer.lineCount() - 1);
    cursor_.column = std::min(cursor_.column, lastColumn(buffer.line(cursor_.row)));
}

void Window::setCursor(const Buffer& buffer, Position position, bool allowLineEnd) {
    cursor_.row = std::min(position.row, buffer.lineCount() - 1);
    const auto& lineText = buffer.line(cursor_.row);
    const auto maximum = allowLineEnd ? lineText.size() : lastColumn(lineText);
    cursor_.column = std::min(position.column, maximum);
    updateDesiredColumn(buffer);
    ensureCursorVisible(buffer);
}

void Window::updateDesiredColumn(const Buffer& buffer) {
    desiredScreenColumn_ = cursorScreenColumn(buffer);
}

void Window::moveLeft(const Buffer& buffer, std::size_t count) {
    cursor_.column = count > cursor_.column ? 0 : cursor_.column - count;
    updateDesiredColumn(buffer);
}

void Window::moveRight(const Buffer& buffer, std::size_t count) {
    const auto maximum = lastColumn(buffer.line(cursor_.row));
    cursor_.column = count > maximum - cursor_.column ? maximum : cursor_.column + count;
    updateDesiredColumn(buffer);
}

void Window::moveWordForward(const Buffer& buffer, std::size_t count) {
    for (std::size_t step = 0; step < count; ++step) {
        const auto original = cursor_;
        const auto& currentLine = buffer.line(cursor_.row);
        bool found = false;

        if (!currentLine.empty()) {
            auto column = std::min(cursor_.column, currentLine.size() - 1);
            const auto initialClass = wordClass(currentLine[column]);
            ++column;

            if (initialClass != WordClass::Whitespace) {
                while (column < currentLine.size() &&
                       wordClass(currentLine[column]) == initialClass) {
                    ++column;
                }
            }
            column = firstNonWhitespace(currentLine, column);

            if (column < currentLine.size()) {
                cursor_.column = column;
                found = true;
            } else if (cursor_.column + 1 < currentLine.size() &&
                       cursor_.row + 1 == buffer.lineCount()) {
                cursor_.column = currentLine.size() - 1;
                found = true;
            }
        }

        if (!found) {
            for (std::size_t row = cursor_.row + 1;
                 row < buffer.lineCount(); ++row) {
                const auto& lineText = buffer.line(row);
                const auto column = firstNonWhitespace(lineText, 0);
                if (column < lineText.size()) {
                    cursor_ = {row, column};
                    found = true;
                    break;
                }
            }
        }

        if (!found ||
            (cursor_.row == original.row &&
             cursor_.column == original.column)) {
            break;
        }
    }
    updateDesiredColumn(buffer);
}

void Window::moveWordBackward(const Buffer& buffer, std::size_t count) {
    for (std::size_t step = 0; step < count; ++step) {
        const auto original = cursor_;
        auto row = cursor_.row;
        std::optional<std::size_t> column;

        const auto& currentLine = buffer.line(row);
        if (!currentLine.empty() && cursor_.column > 0) {
            column = std::min(cursor_.column - 1, currentLine.size() - 1);
        }

        while (true) {
            if (column.has_value()) {
                while (wordClass(buffer.line(row)[*column]) ==
                       WordClass::Whitespace) {
                    if (*column == 0) {
                        column.reset();
                        break;
                    }
                    --*column;
                }

                if (column.has_value()) {
                    const auto targetClass =
                        wordClass(buffer.line(row)[*column]);
                    while (*column > 0 &&
                           wordClass(buffer.line(row)[*column - 1]) ==
                               targetClass) {
                        --*column;
                    }
                    cursor_ = {row, *column};
                    break;
                }
            }

            if (row == 0) {
                break;
            }
            --row;
            const auto& previousLine = buffer.line(row);
            if (!previousLine.empty()) {
                column = previousLine.size() - 1;
            }
        }

        if (cursor_.row == original.row &&
            cursor_.column == original.column) {
            break;
        }
    }
    updateDesiredColumn(buffer);
}

void Window::moveWordEnd(const Buffer& buffer, std::size_t count) {
    for (std::size_t step = 0; step < count; ++step) {
        const auto original = cursor_;
        auto row = cursor_.row;
        const auto& currentLine = buffer.line(row);
        std::size_t searchColumn = 0;
        bool found = false;

        if (!currentLine.empty()) {
            const auto column =
                std::min(cursor_.column, currentLine.size() - 1);
            const auto currentClass = wordClass(currentLine[column]);

            if (currentClass != WordClass::Whitespace) {
                auto end = column;
                while (end + 1 < currentLine.size() &&
                       wordClass(currentLine[end + 1]) == currentClass) {
                    ++end;
                }
                if (end > column) {
                    cursor_.column = end;
                    found = true;
                }
            }
            searchColumn = column + 1;
        }

        while (!found && row < buffer.lineCount()) {
            const auto& lineText = buffer.line(row);
            const auto start = firstNonWhitespace(lineText, searchColumn);
            if (start < lineText.size()) {
                const auto targetClass = wordClass(lineText[start]);
                auto end = start;
                while (end + 1 < lineText.size() &&
                       wordClass(lineText[end + 1]) == targetClass) {
                    ++end;
                }
                cursor_ = {row, end};
                found = true;
                break;
            }
            ++row;
            searchColumn = 0;
        }

        if (!found ||
            (cursor_.row == original.row &&
             cursor_.column == original.column)) {
            break;
        }
    }
    updateDesiredColumn(buffer);
}

void Window::moveUp(const Buffer& buffer, std::size_t count) {
    const auto target = count > cursor_.row ? 0 : cursor_.row - count;
    moveVerticallyTo(buffer, target);
}

void Window::moveDown(const Buffer& buffer, std::size_t count) {
    const auto maximum = buffer.lineCount() - 1;
    const auto target = count > maximum - cursor_.row ? maximum : cursor_.row + count;
    moveVerticallyTo(buffer, target);
}

void Window::moveVerticallyTo(const Buffer& buffer, std::size_t row) {
    cursor_.row = row;
    cursor_.column = text::bufferColumn(buffer.line(cursor_.row), desiredScreenColumn_);
}

void Window::moveToLineEdge(const Buffer& buffer, bool firstNonBlankOnly) {
    cursor_.column = firstNonBlankOnly ? firstNonBlank(buffer.line(cursor_.row)) : 0;
    updateDesiredColumn(buffer);
}

} // namespace sjtu
