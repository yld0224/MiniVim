#ifndef MINIVIM_WINDOW_HPP
#define MINIVIM_WINDOW_HPP

#include "Buffer.hpp"
#include "Types.hpp"


namespace sjtu {

class Window {

public:
    void Resize(ScreenSize terminal_size);

    void ApplyMotion(const Buffer& buffer, Motion motion);

    void EnsureCursorVisible(const Buffer& buffer);

    void SetCursor(const Buffer& buffer, Position position, bool allow_line_end);
    const Position& GetCursor() const ;
    const Viewport& GetViewport() const ;

private:
    void MoveLeft(const Buffer& buffer, size_t count);
    void MoveRight(const Buffer& buffer, size_t count);
    void MoveUp(const Buffer& buffer, size_t count);
    void MoveDown(const Buffer& buffer, size_t count);

    Position cursor_{};
    Viewport viewport_{};
    size_t desired_screen_column_{0};
};

} // namespace sjtu

#endif // MINIVIM_WINDOW_HPP
