#ifndef MINIVIM_RENDERER_HPP
#define MINIVIM_RENDERER_HPP

#include "Buffer.hpp"
#include "Types.hpp"
#include "Window.hpp"

#include <string>
#include <string_view>

namespace sjtu {

struct RenderState {
    Mode mode{Mode::Normal};
    std::string_view commandLine;
    std::string_view message;
    std::string_view pendingKeys;
};

// Renderer is deliberately free of terminal I/O and mutable editor state. It
// turns a snapshot into one ANSI frame that Terminal writes atomically.
class Renderer {
public:
    [[nodiscard]] std::string render(const Buffer& buffer,
                                     const Window& window,
                                     const RenderState& state) const;

private:
    [[nodiscard]] static std::string statusLine(const Buffer& buffer,
                                                const Window& window,
                                                Mode mode);
    [[nodiscard]] static std::string fitLine(std::string left,
                                             std::string right,
                                             std::size_t width);
    [[nodiscard]] static std::string_view modeName(Mode mode) noexcept;
};

} // namespace sjtu

#endif // MINIVIM_RENDERER_HPP
