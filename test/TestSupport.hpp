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

inline void Fail(const char* expression, const char* file, int line,
                 std::string_view detail = {}) {
    std::ostringstream message;
    message << file << ':' << line << ": check failed: " << expression;
    if (!detail.empty()) {
        message << " (" << detail << ')';
    }
    throw Failure(message.str());
}

inline void CheckContains(std::string_view text, std::string_view expected,
                          const char* file, int line) {
    if (text.find(expected) == std::string_view::npos) {
        Fail("text contains expected", file, line,
             "missing `" + std::string(expected) + "`");
    }
}

template <typename Exception, typename Function>
std::string ExpectThrows(Function&& function, const char* expression,
                         const char* file, int line) {
    try {
        std::forward<Function>(function)();
    } catch (const Exception& error) {
        return error.what();
    } catch (const std::exception& error) {
        Fail(expression, file, line,
             "wrong exception type: " + std::string(error.what()));
    } catch (...) {
        Fail(expression, file, line, "wrong non-standard exception type");
    }
    Fail(expression, file, line, "no exception was thrown");
    return {};
}

class TempDirectory {
public:
    TempDirectory() {
        const auto base = std::filesystem::temp_directory_path();
        for (unsigned int attempt = 0; attempt < 1000; ++attempt) {
            const auto candidate =
                base / ("minivim-basic-test-" + std::to_string(::getpid()) +
                        '-' + std::to_string(attempt));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) {
                path_ = candidate;
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

    const std::filesystem::path& Path() const noexcept { return path_; }

    std::filesystem::path File(std::string_view name) const {
        return path_ / std::string(name);
    }

private:
    std::filesystem::path path_;
};

inline void WriteFile(const std::filesystem::path& path,
                      std::string_view contents) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("cannot create test file " + path.string());
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!output) {
        throw std::runtime_error("cannot write test file " + path.string());
    }
}

inline std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot read test file " + path.string());
    }
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

struct TestCase {
    const char* name;
    std::function<void()> body;
};

inline int Run(std::initializer_list<TestCase> cases) {
    std::size_t passed = 0;
    for (const auto& test_case : cases) {
        try {
            test_case.body();
            ++passed;
            std::cout << "[PASS] " << test_case.name << '\n' << std::flush;
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << test_case.name << "\n  "
                      << error.what() << '\n';
        } catch (...) {
            std::cerr << "[FAIL] " << test_case.name
                      << "\n  unknown exception\n";
        }
    }

    std::cout << passed << '/' << cases.size() << " tests passed\n"
              << std::flush;
    return passed == cases.size() ? 0 : 1;
}

} // namespace test

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            ::test::Fail(#condition, __FILE__, __LINE__);                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(actual, expected) CHECK((actual) == (expected))
#define CHECK_NE(actual, expected) CHECK((actual) != (expected))
#define CHECK_CONTAINS(text, expected)                                          \
    ::test::CheckContains((text), (expected), __FILE__, __LINE__)
#define EXPECT_THROWS(exception, expression)                                    \
    ::test::ExpectThrows<exception>([&] { static_cast<void>(expression); },      \
                                    #expression, __FILE__, __LINE__)

#endif // MINIVIM_TEST_SUPPORT_HPP
