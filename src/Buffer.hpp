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
    explicit Buffer(std::vector<std::string> lines,
                    std::filesystem::path path = {});

    [[nodiscard]] std::size_t lineCount() const noexcept;
    [[nodiscard]] const std::string& line(std::size_t row) const;
    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] std::string displayName() const;

private:
    void ensureNonEmpty();

    // Keeping this private ensures future editing and undo code has one place
    // to maintain buffer invariants.
    std::vector<std::string> lines_;
    std::filesystem::path path_;
};

} // namespace sjtu

#endif // MINIVIM_BUFFER_HPP
