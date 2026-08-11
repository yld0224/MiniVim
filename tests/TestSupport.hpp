#ifndef MINIVIM_TEST_SUPPORT_HPP
#define MINIVIM_TEST_SUPPORT_HPP

#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <unistd.h>

namespace test {

class Failure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline void fail(const char* expression, const char* file, int line,
                 std::string_view detail = {}) {
    std::ostringstream message;
    message << file << ':' << line << ": check failed: " << expression;
    if (!detail.empty()) {
        message << " (" << detail << ')';
    }
    throw Failure(message.str());
}

inline void checkContains(std::string_view text, std::string_view expected,
                          const char* file, int line) {
    if (text.find(expected) == std::string_view::npos) {
        fail("text contains expected", file, line,
             "missing `" + std::string(expected) + "`");
    }
}

template <typename Exception, typename Function>
std::string expectThrows(Function&& function, const char* expression,
                         const char* file, int line) {
    try {
        std::forward<Function>(function)();
    } catch (const Exception& error) {
        return error.what();
    } catch (const std::exception& error) {
        fail(expression, file, line,
             "wrong exception type: " + std::string(error.what()));
    } catch (...) {
        fail(expression, file, line, "wrong non-standard exception type");
    }
    fail(expression, file, line, "no exception was thrown");
    return {};
}

class TempDirectory {
public:
    TempDirectory() {
        const auto base = std::filesystem::temp_directory_path();
        for (unsigned int attempt = 0; attempt < 1000; ++attempt) {
            auto candidate = base / ("minivim-test-" + std::to_string(::getpid()) +
                                     '-' + std::to_string(attempt));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) {
                path_ = std::move(candidate);
                return;
            }
            if (error && error != std::errc::file_exists) {
                throw std::filesystem::filesystem_error(
                    "cannot create temporary test directory", candidate, error);
            }
        }
        throw std::runtime_error("cannot allocate temporary test directory");
    }

    ~TempDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    const std::filesystem::path& path() const noexcept { return path_; }

    std::filesystem::path file(std::string_view name) const {
        return path_ / std::string(name);
    }

private:
    std::filesystem::path path_;
};

inline void writeFile(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("cannot create test file " + path.string());
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!output) {
        throw std::runtime_error("cannot write test file " + path.string());
    }
}

inline std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot read test file " + path.string());
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

struct TestCase {
    const char* name;
    std::function<void()> body;
};

inline int run(std::initializer_list<TestCase> cases) {
    std::size_t passed = 0;
    for (const auto& testCase : cases) {
        try {
            testCase.body();
            ++passed;
            std::cout << "[PASS] " << testCase.name << '\n' << std::flush;
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << testCase.name << "\n  " << error.what() << '\n';
        } catch (...) {
            std::cerr << "[FAIL] " << testCase.name << "\n  unknown exception\n";
        }
    }

    std::cout << passed << '/' << cases.size() << " tests passed\n" << std::flush;
    return passed == cases.size() ? 0 : 1;
}

} // namespace test

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            ::test::fail(#condition, __FILE__, __LINE__);                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(actual, expected) CHECK((actual) == (expected))
#define CHECK_NE(actual, expected) CHECK((actual) != (expected))
#define CHECK_CONTAINS(text, expected)                                          \
    ::test::checkContains((text), (expected), __FILE__, __LINE__)
#define EXPECT_THROWS(exception, expression)                                    \
    ::test::expectThrows<exception>([&] { static_cast<void>(expression); },      \
                                    #expression, __FILE__, __LINE__)

#endif // MINIVIM_TEST_SUPPORT_HPP
