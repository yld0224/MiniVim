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
    bool isModified() const noexcept;

    void insertCharacter(std::size_t row, std::size_t column, char value);
    void eraseCharacter(std::size_t row, std::size_t column);
    void splitLine(std::size_t row, std::size_t column);
    void joinWithNextLine(std::size_t row);
    void save();

private:
    void ensureNonEmpty();

    std::vector<std::string> lines_;
    std::filesystem::path path_;
    bool endsWithNewline_{false};
    bool modified_{false};
};

} // namespace sjtu

#endif // MINIVIM_BUFFER_HPP
