#include "Window.hpp"
#include "TextLayout.hpp"

#include <algorithm>
#include <cctype>

namespace sjtu {


void Window::Resize(ScreenSize terminal_size) {
    viewport_.columns_ = std::max<size_t>(terminal_size.columns_, 1);
    viewport_.rows_ = terminal_size.rows_ > 1 ? terminal_size.rows_ - 1 : 1;
}

void Window::ApplyMotion(const Buffer& buffer, Motion motion) {
    switch (motion) {
        case Motion::Left:
            MoveLeft(buffer, 1);
            break;
        case Motion::Right:
            MoveRight(buffer, 1);
            break;
        case Motion::Up:
            MoveUp(buffer, 1);
            break;
        case Motion::Down:
            MoveDown(buffer, 1);
            break;
        }

    cursor_.row_ = std::min(cursor_.row_, buffer.GetLineCount() - 1);
    cursor_.column_ = std::min(cursor_.column_, text::LastColumn(buffer.GetLineAt(cursor_.row_)));
    EnsureCursorVisible(buffer);
}

void Window::EnsureCursorVisible(const Buffer& buffer) {

    if (cursor_.row_ < viewport_.top_) {
        viewport_.top_ = cursor_.row_;
    } else if (cursor_.row_ >= viewport_.top_ + viewport_.rows_) {
        viewport_.top_ = cursor_.row_ - viewport_.rows_ + 1;
    }

    size_t screen_column = text::BufferColumnToRenderColumn(buffer.GetLineAt(cursor_.row_), cursor_.column_);
    if (screen_column < viewport_.left_) {
        viewport_.left_ = screen_column;
    } else if (screen_column >= viewport_.left_ + viewport_.columns_) {
        viewport_.left_ = screen_column - viewport_.columns_ + 1;
    }
}

const Position& Window::GetCursor() const {
    return cursor_;
}

const Viewport& Window::GetViewport() const {
    return viewport_;
}


void Window::SetCursor(const Buffer& buffer, Position position, bool allow_line_end) {
    cursor_.row_ = std::min(position.row_, buffer.GetLineCount() - 1);
    auto& line = buffer.GetLineAt(cursor_.row_);
    auto maximum = allow_line_end ? line.size() : text::LastColumn(line);
    cursor_.column_ = std::min(position.column_, maximum);
    
    desired_screen_column_ = text::BufferColumnToRenderColumn(buffer.GetLineAt(cursor_.row_), cursor_.column_);
    EnsureCursorVisible(buffer);
}


void Window::MoveLeft(const Buffer& buffer, std::size_t count) {
    cursor_.column_ = count > cursor_.column_ ? 0 : cursor_.column_ - count;
    desired_screen_column_ = text::BufferColumnToRenderColumn(buffer.GetLineAt(cursor_.row_), cursor_.column_);
}

void Window::MoveRight(const Buffer& buffer, std::size_t count) {
    size_t maximum = text::LastColumn(buffer.GetLineAt(cursor_.row_));
    cursor_.column_ = count > maximum - cursor_.column_ ? maximum : cursor_.column_ + count;
    desired_screen_column_ = text::BufferColumnToRenderColumn(buffer.GetLineAt(cursor_.row_), cursor_.column_);
}


void Window::MoveUp(const Buffer& buffer, std::size_t count) {
    size_t target = count > cursor_.row_ ? 0 : cursor_.row_ - count;
    cursor_.row_ = target;
    cursor_.column_ = text::RenderColumnToBufferColumn(buffer.GetLineAt(cursor_.row_), desired_screen_column_);
}

void Window::MoveDown(const Buffer& buffer, std::size_t count) {
    size_t maximum = buffer.GetLineCount() - 1;
    auto target = count > maximum - cursor_.row_ ? maximum : cursor_.row_ + count;
    cursor_.row_ = target;
    cursor_.column_ = text::RenderColumnToBufferColumn(buffer.GetLineAt(cursor_.row_), desired_screen_column_);
}

} // namespace sjtu
