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
        endsWithNewline_ = !file.eof();
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

bool Buffer::isModified() const noexcept {
    return modified_;
}

void Buffer::insertCharacter(std::size_t row, std::size_t column, char value) {
    auto& lineText = lines_.at(row);
    if (column > lineText.size()) {
        throw std::out_of_range("column out of range");
    }
    lineText.insert(column, 1, value);
    modified_ = true;
}

void Buffer::eraseCharacter(std::size_t row, std::size_t column) {
    auto& lineText = lines_.at(row);
    if (column >= lineText.size()) {
        throw std::out_of_range("column out of range");
    }
    lineText.erase(column, 1);
    modified_ = true;
}

void Buffer::splitLine(std::size_t row, std::size_t column) {
    const auto& lineText = lines_.at(row);
    if (column > lineText.size()) {
        throw std::out_of_range("column out of range");
    }

    const auto remainder = lineText.substr(column);
    const auto next = lines_.begin() +
                      static_cast<std::vector<std::string>::difference_type>(row + 1);
    lines_.insert(next, remainder);
    lines_[row].erase(column);
    modified_ = true;
}

void Buffer::joinWithNextLine(std::size_t row) {
    if (row + 1 >= lines_.size()) {
        throw std::out_of_range("line out of range");
    }

    lines_[row] += lines_[row + 1];
    const auto next = lines_.begin() +
                      static_cast<std::vector<std::string>::difference_type>(row + 1);
    lines_.erase(next);
    modified_ = true;
}

void Buffer::save() {
    if (path_.empty()) {
        throw std::runtime_error("no file name");
    }

    std::ofstream file(path_, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("cannot write " + path_.string());
    }

    for (std::size_t row = 0; row < lines_.size(); ++row) {
        if (row > 0) {
            file.put('\n');
        }
        file << lines_[row];
    }
    if (endsWithNewline_) {
        file.put('\n');
    }

    file.close();
    if (!file) {
        throw std::runtime_error("cannot write " + path_.string());
    }
    modified_ = false;
}

void Buffer::ensureNonEmpty() {
    if (lines_.empty()) {
        lines_.emplace_back();
    }
}

} // namespace sjtu
