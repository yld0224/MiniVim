#include "Window.hpp"
#include "TextLayout.hpp"

namespace sjtu {

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
