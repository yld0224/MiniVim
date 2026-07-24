#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vector>
#include <string>
#include <fstream>

namespace sjtu{

class Buffer{
private:
    std::vector<std::string> rows_;
    std::string filename_ = "[Unnamed]";
public:
    Buffer(const std::string& filename) {
        if (filename.empty()) {
            return;
        }
        filename_ = filename;
	    std::ifstream file(filename);
	    if (!file.is_open()) {
		    throw std::runtime_error("open");
	    }
	    std::string line;
	    while (std::getline(file, line)) {
		    if (!line.empty() && line.back() == '\r') {
			    line.pop_back();
		    }
		    rows_.emplace_back(line);
	    }
	    if (file.bad()) {throw std::runtime_error("read file");}
    }

    size_t rowSize() {
        return rows_.size();
    }

    size_t colSize(size_t row) {
        return rows_[row].size();
    }

    std::string* getRow(std::size_t row) {
        if (row < rows_.size()) {
            return &rows_[row];
        }
        return nullptr;
    }

    const std::string& getFilename() const {return filename_;}
};
}
#endif //BUFFER_HPP