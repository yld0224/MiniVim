#ifndef MINIVIM_BUFFER_HPP
#define MINIVIM_BUFFER_HPP

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace sjtu {

class Buffer {
public:
    explicit Buffer(const std::filesystem::path& path = {});
    explicit Buffer(std::vector<std::string> lines, std::filesystem::path path = {});

    std::size_t lineCount() const noexcept;
    const std::string& line(std::size_t row) const;
    const std::filesystem::path& path() const noexcept;
    std::string displayName() const;

private:
    void ensureNonEmpty();

    std::vector<std::string> lines_;
    std::filesystem::path path_;
};

} // namespace sjtu

#endif // MINIVIM_BUFFER_HPP
