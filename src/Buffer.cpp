#include "Buffer.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace sjtu {

Buffer::Buffer(const std::filesystem::path& path) : path_(path) {
    if (path_.empty()) {
        ensureNonEmpty();
        return;
    }

    std::ifstream file(path_);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open " + path_.string());
    }

    std::string lineText;
    while (std::getline(file, lineText)) {
        if (!lineText.empty() && lineText.back() == '\r') {
            lineText.pop_back();
        }
        lines_.push_back(std::move(lineText));
    }
    if (file.bad()) {
        throw std::runtime_error("cannot read " + path_.string());
    }
    ensureNonEmpty();
}

Buffer::Buffer(std::vector<std::string> lines, std::filesystem::path path) : lines_(std::move(lines)), path_(std::move(path)) {
    ensureNonEmpty();
}

std::size_t Buffer::lineCount() const noexcept {
    return lines_.size();
}

const std::string& Buffer::line(std::size_t row) const {
    return lines_.at(row);
}

const std::filesystem::path& Buffer::path() const noexcept {
    return path_;
}

std::string Buffer::displayName() const {
    return path_.empty() ? "[No Name]" : path_.string();
}

void Buffer::ensureNonEmpty() {
    if (lines_.empty()) {
        lines_.emplace_back();
    }
}

} // namespace sjtu
