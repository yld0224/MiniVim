#include <fstream>

#include "Buffer.hpp"

namespace sjtu {

Buffer::Buffer(const std::filesystem::path& path) : path_(path) {
    if (path_.empty()) {
        EnsureNonEmpty();
        return;
    }

    std::ifstream file(path_);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open " + path_.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        ends_with_newline_ = !file.eof();
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines_.push_back(std::move(line));
    }
    if (file.bad()) {
        throw std::runtime_error("cannot read " + path_.string());
    }
    EnsureNonEmpty();
}

Buffer::Buffer(std::vector<std::string> lines, std::filesystem::path path) : lines_(std::move(lines)), path_(std::move(path)) {
    EnsureNonEmpty();
}

std::size_t Buffer::GetLineCount() const {
    return lines_.size();
}

const std::string& Buffer::GetLineAt(std::size_t row) const {
    return lines_.at(row);
}


std::string Buffer::GetDisplayName() const {
    return path_.empty() ? "[No Name]" : path_.string();
}

bool Buffer::IsModified() const {
    return modified_;
}

void Buffer::InsertCharacter(std::size_t row, std::size_t column, char value) {
    auto& line = lines_.at(row);
    if (column > line.size()) {
        throw std::out_of_range("column out of range");
    }
    line.insert(column, 1, value);
    modified_ = true;
}

void Buffer::EraseCharacter(std::size_t row, std::size_t column) {
    auto& line = lines_.at(row);
    if (column >= line.size()) {
        throw std::out_of_range("column out of range");
    }
    line.erase(column, 1);
    modified_ = true;
}

void Buffer::SplitLine(std::size_t row, std::size_t column) {
    auto& lineText = lines_.at(row);
    if (column > lineText.size()) {
        throw std::out_of_range("column out of range");
    }

    auto remainder = lineText.substr(column);
    auto next = lines_.begin() + static_cast<std::vector<std::string>::difference_type>(row + 1);
    lines_.insert(next, remainder);
    lines_[row].erase(column);
    modified_ = true;
}

void Buffer::JoinLine(std::size_t row) {
    if (row + 1 >= lines_.size()) {
        throw std::out_of_range("line out of range");
    }

    lines_[row] += lines_[row + 1];
    auto next = lines_.begin() + static_cast<std::vector<std::string>::difference_type>(row + 1);
    lines_.erase(next);
    modified_ = true;
}

void Buffer::Save() {
    if (path_.empty()) {
        throw std::runtime_error("no file name");
    }

    WriteTo(path_);
    modified_ = false;
}

void Buffer::SaveAs(const std::filesystem::path& path) {
    if (path.empty()) {
        throw std::runtime_error("no file name");
    }

    WriteTo(path);
    path_ = path;
    modified_ = false;
}

void Buffer::EnsureNonEmpty() {
    if (lines_.empty()) {
        lines_.emplace_back();
    }
}

void Buffer::WriteTo(const std::filesystem::path& path) const {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("cannot write " + path.string());
    }

    for (std::size_t row = 0; row < lines_.size(); ++row) {
        if (row > 0) {
            file.put('\n');
        }
        file << lines_[row];
    }
    if (ends_with_newline_) {
        file.put('\n');
    }

    file.close();
    if (!file) {
        throw std::runtime_error("cannot write " + path.string());
    }
}

} // namespace sjtu
