#include "Renderer.hpp"

#include "TextLayout.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace sjtu {
namespace {

void appendClearedLine(std::string& frame, std::string_view contents,
                       std::size_t width, bool newline) {
    frame.append(contents.substr(0, width));
    frame += "\x1b[K";
    if (newline) {
        frame += "\r\n";
    }
}

std::string cursorSequence(std::size_t row, std::size_t column) {
    return "\x1b[" + std::to_string(row) + ';' +
           std::to_string(column) + 'H';
}

} // namespace

std::string Renderer::render(const Buffer& buffer, const Window& window,
                             const RenderState& state) const {
    const auto& viewport = window.viewport();
    const auto width = std::max<std::size_t>(viewport.columns, 1);

    std::string frame;
    frame.reserve((viewport.textRows + 2) * (width + 8));
    frame += "\x1b[?25l";
    frame += "\x1b[H";

    for (std::size_t screenRow = 0; screenRow < viewport.textRows;
         ++screenRow) {
        const auto bufferRow = viewport.top + screenRow;
        if (bufferRow >= buffer.lineCount()) {
            appendClearedLine(frame, "~", width, true);
            continue;
        }

        const auto rendered = text::expandForDisplay(buffer.line(bufferRow));
        const auto visible =
            viewport.left < rendered.size()
                ? std::string_view(rendered).substr(viewport.left, width)
                : std::string_view{};
        appendClearedLine(frame, visible, width, true);
    }

    frame += "\x1b[7m";
    frame += statusLine(buffer, window, state.mode);
    frame += "\x1b[m";
    frame += "\r\n";

    std::string bottomLeft;
    std::string bottomRight;
    if (state.mode == Mode::CommandLine) {
        bottomLeft = ":" + std::string(state.commandLine);
    } else {
        bottomLeft = std::string(state.message);
        bottomRight = std::string(state.pendingKeys);
    }
    appendClearedLine(frame,
                      fitLine(std::move(bottomLeft), std::move(bottomRight),
                              width),
                      width, false);

    std::size_t cursorRow = 1;
    std::size_t cursorColumn = 1;
    if (state.mode == Mode::CommandLine) {
        cursorRow = viewport.textRows + 2;
        cursorColumn = std::min<std::size_t>(state.commandLine.size() + 2,
                                             width);
    } else {
        cursorRow = window.cursor().row - viewport.top + 1;
        cursorColumn =
            window.cursorScreenColumn(buffer) - viewport.left + 1;
        cursorColumn = std::min(cursorColumn, width);
    }

    frame += cursorSequence(cursorRow, cursorColumn);
    frame += "\x1b[?25h";
    return frame;
}

std::string Renderer::statusLine(const Buffer& buffer, const Window& window,
                                 Mode mode) {
    const auto left = " " + std::string(modeName(mode)) + "  " +
                      text::expandForDisplay(buffer.displayName()) + " [RO]";
    const auto right =
        std::to_string(window.cursor().row + 1) + ',' +
        std::to_string(window.cursor().column + 1) + ' ';
    return fitLine(left, right, window.viewport().columns);
}

std::string Renderer::fitLine(std::string left, std::string right,
                              std::size_t width) {
    if (width == 0) {
        return {};
    }
    if (right.size() >= width) {
        return right.substr(right.size() - width);
    }

    const auto leftLimit = width - right.size();
    if (left.size() > leftLimit) {
        left.resize(leftLimit);
    }
    left.append(leftLimit - left.size(), ' ');
    left += right;
    return left;
}

std::string_view Renderer::modeName(Mode mode) noexcept {
    switch (mode) {
    case Mode::Normal:
        return "NORMAL";
    case Mode::Insert:
        return "INSERT";
    case Mode::CommandLine:
        return "COMMAND";
    }
    return "UNKNOWN";
}

} // namespace sjtu
