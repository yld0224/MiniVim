#ifndef MINIVIM_BUFFER_HPP
#define MINIVIM_BUFFER_HPP

#include <filesystem>
#include <vector>

namespace sjtu {

class Buffer {

public:
    explicit Buffer(const std::filesystem::path& path = {});
    explicit Buffer(std::vector<std::string> lines, std::filesystem::path path = {});

    std::size_t GetLineCount() const;
    const std::string& GetLineAt(std::size_t row) const;
    std::string GetDisplayName() const;
    bool IsModified() const ;

    void InsertCharacter(std::size_t row, std::size_t column, char value);
    void EraseCharacter(std::size_t row, std::size_t column);
    void SplitLine(std::size_t row, std::size_t column);
    void JoinLine(std::size_t row);
    void Save();
    void SaveAs(const std::filesystem::path& path);

private:
    void EnsureNonEmpty();
    void WriteTo(const std::filesystem::path& path) const;

    std::vector<std::string> lines_;
    std::filesystem::path path_;
    bool ends_with_newline_{false};
    bool modified_{false};
};

} // namespace sjtu

#endif // MINIVIM_BUFFER_HPP
