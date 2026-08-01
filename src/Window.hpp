#ifndef MINIVIM_WINDOW_HPP
#define MINIVIM_WINDOW_HPP

#include "Buffer.hpp"
#include "Types.hpp"

#include <optional>

namespace sjtu {

// Window owns view-local state. Keeping cursor and viewport out of Buffer makes
// it possible to add multiple views of the same text later.
class Window {
public:
    void resize(ScreenSize terminalSize);
    void applyMotion(const Buffer& buffer, Motion motion, std::optional<std::size_t> count = std::nullopt);
    void ensureCursorVisible(const Buffer& buffer);
    void setNormalCursor(const Buffer& buffer, Position position);
    void setInsertCursor(const Buffer& buffer, Position position);

    const Position& cursor() const noexcept;
    const Viewport& viewport() const noexcept;
    std::size_t cursorScreenColumn(const Buffer& buffer) const;

private:
    static std::size_t lastColumn(const std::string& line) noexcept;
    static std::size_t firstNonBlank(const std::string& line) noexcept;
    static std::size_t scaledStep(std::size_t step, std::size_t count) noexcept;

    void normalize(const Buffer& buffer);
    void setCursor(const Buffer& buffer, Position position, bool allowLineEnd);
    void updateDesiredColumn(const Buffer& buffer);
    void moveLeft(const Buffer& buffer, std::size_t count);
    void moveRight(const Buffer& buffer, std::size_t count);
    void moveWordForward(const Buffer& buffer, std::size_t count);
    void moveWordBackward(const Buffer& buffer, std::size_t count);
    void moveWordEnd(const Buffer& buffer, std::size_t count);
    void moveUp(const Buffer& buffer, std::size_t count);
    void moveDown(const Buffer& buffer, std::size_t count);
    void moveVerticallyTo(const Buffer& buffer, std::size_t row);
    void moveToLineEdge(const Buffer& buffer, bool firstNonBlankOnly);

    Position cursor_{};
    Viewport viewport_{};
    std::size_t desiredScreenColumn_{0};
};

} // namespace sjtu

#endif // MINIVIM_WINDOW_HPP
