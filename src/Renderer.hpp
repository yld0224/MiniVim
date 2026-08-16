#ifndef MINIVIM_RENDERER_HPP
#define MINIVIM_RENDERER_HPP

#include "Buffer.hpp"
#include "Types.hpp"
#include "Window.hpp"

#include <string>

namespace sjtu {

struct RenderState {
    Mode mode_{Mode::Normal};
    std::string command_;
    std::string message_;
};

class Renderer {
public:
    std::string Render(const Buffer& buffer, const Window& window, const RenderState& state) const;

private:
    static std::string StatusLine(const Buffer& buffer, const Window& window, Mode mode);
    static std::string FitLine(std::string left, std::string right, std::size_t width);
    static std::string ExpandForDisplay(std::string_view line); 
    static std::string GetModeName(Mode mode);
};

} // namespace sjtu

#endif // MINIVIM_RENDERER_HPP
