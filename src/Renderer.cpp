#include "Renderer.hpp"
#include "TextLayout.hpp"

namespace sjtu {
    
namespace {

void AppendClearedLine(std::string& frame, std::string_view contents, std::size_t width, bool newline) {
    frame.append(contents.substr(0, width));
    frame += "\x1b[K";
    if (newline) {
        frame += "\r\n";
    }
}

std::string CursorSequence(std::size_t row, std::size_t column) {
    return "\x1b[" + std::to_string(row) + ';' + std::to_string(column) + 'H';
}

}

std::string Renderer::Render(const Buffer& buffer, const Window& window, const RenderState& state) const {
    auto& viewport = window.GetViewport();
    auto width = std::max<std::size_t>(viewport.columns_, 1);

    std::string frame;
    frame.reserve((viewport.rows_ + 2) * (width + 8));
    frame += "\x1b[?25l";
    frame += "\x1b[H";

    for (std::size_t screen_row = 0; screen_row < viewport.rows_; ++screen_row) {
        auto buffer_row = viewport.top_ + screen_row;
        if (buffer_row >= buffer.GetLineCount()) {
            AppendClearedLine(frame, "~", width, true);
            continue;
        }

        auto rendered = ExpandForDisplay(buffer.GetLineAt(buffer_row));
        auto visible = viewport.left_ < rendered.size() ? std::string_view(rendered).substr(viewport.left_, width) : std::string_view{};
        AppendClearedLine(frame, visible, width, true);
    }

    frame += "\x1b[7m";
    frame += StatusLine(buffer, window, state.mode_);
    frame += "\x1b[m";
    frame += "\r\n";

    std::string bottom_left;
    std::string bottom_right;
    if (state.mode_ == Mode::CommandLine) {
        bottom_left = ":" + std::string(state.command_);
    } else {
        bottom_left = std::string(state.message_);
    }
    AppendClearedLine(frame, FitLine(std::move(bottom_left), std::move(bottom_right), width), width, false);

    std::size_t cursor_row{0};
    std::size_t cursor_column{0};
    if (state.mode_ == Mode::CommandLine) {
        cursor_row = viewport.rows_ + 2;
        cursor_column = std::min<std::size_t>(state.command_.size() + 2, width);
    } else {
        cursor_row = window.GetCursor().row_ - viewport.top_ + 1;
        cursor_column = text::BufferColumnToRenderColumn(buffer.GetLineAt(window.GetCursor().row_), window.GetCursor().column_) - viewport.left_ + 1;
        cursor_column = std::min(cursor_column, width);
    }

    frame += CursorSequence(cursor_row, cursor_column);
    frame += "\x1b[?25h";
    return frame;
}

std::string Renderer::StatusLine(const Buffer& buffer, const Window& window, Mode mode) {
    auto left = " " + std::string(GetModeName(mode)) + "  " + ExpandForDisplay(buffer.GetDisplayName());
    if (buffer.IsModified()) {
        left += " [+]";
    }
    auto right = std::to_string(window.GetCursor().row_ + 1) + ',' + std::to_string(window.GetCursor().column_ + 1) + ' ';
    return FitLine(left, right, window.GetViewport().columns_);
}

std::string Renderer::FitLine(std::string left, std::string right, std::size_t width) {
    if (width == 0) { return {}; }
    if (right.size() >= width) { return right.substr(right.size() - width); }

    auto left_limit = width - right.size();
    if (left.size() > left_limit) { left.resize(left_limit); }
    left.append(left_limit - left.size(), ' ');
    left += right;
    return left;
}

std::string Renderer::GetModeName(Mode mode)  {
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

std::string Renderer::ExpandForDisplay(std::string_view line) {
    std::string rendered;
    rendered.reserve(line.size());

    size_t column = 0;
    for (char raw : line) {
        auto value = static_cast<unsigned char>(raw);
        if (value == '\t') {
            size_t next = text::NextScreenColumn(column, value);
            rendered.append(next - column, ' ');
            column = next;
        } else if (value < 0x20U || value == 0x7FU) {
            rendered.push_back('^');
            rendered.push_back( value == 0x7FU ? '?' : static_cast<char>(value + 0x40U));
            column += 2;
        } else {
            rendered.push_back(raw);
            ++column;
        }
    }

    return rendered;
}
} // namespace sjtu
