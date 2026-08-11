#ifndef MINIVIM_PTY_SUPPORT_HPP
#define MINIVIM_PTY_SUPPORT_HPP

#include "TestSupport.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace test {

struct InputChunk {
    std::string bytes;
    int pauseAfterMilliseconds{0};
};

struct PtyResult {
    int exitCode{-1};
    int signal{0};
    bool timedOut{false};
    bool becameReady{false};
    std::string output;
    std::string report;
};

using PtyChild = std::function<void(int readyFileDescriptor)>;

inline void writeAll(int fileDescriptor, std::string_view bytes) {
    std::size_t written = 0;
    while (written < bytes.size()) {
        const auto result = ::write(fileDescriptor, bytes.data() + written,
                                    bytes.size() - written);
        if (result > 0) {
            written += static_cast<std::size_t>(result);
            continue;
        }
        if (result == -1 && errno == EINTR) {
            continue;
        }
        if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd descriptor{fileDescriptor, POLLOUT, 0};
            static_cast<void>(::poll(&descriptor, 1, 20));
            continue;
        }
        throw std::system_error(errno, std::generic_category(), "write");
    }
}

inline void signalReady(int fileDescriptor) {
    writeAll(fileDescriptor, "R");
}

inline bool appendAvailable(int fileDescriptor, std::string& destination) {
    char buffer[4096];
    while (true) {
        const auto result = ::read(fileDescriptor, buffer, sizeof(buffer));
        if (result > 0) {
            destination.append(buffer, static_cast<std::size_t>(result));
            continue;
        }
        if (result == 0) {
            return false;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;
        }
        if (errno == EIO) {
            return false;
        }
        return false;
    }
}

inline void makeNonBlocking(int fileDescriptor) {
    const auto flags = ::fcntl(fileDescriptor, F_GETFL);
    if (flags != -1) {
        static_cast<void>(::fcntl(fileDescriptor, F_SETFL, flags | O_NONBLOCK));
    }
}

inline PtyResult runInPty(const PtyChild& child,
                          const std::vector<InputChunk>& input = {},
                          std::size_t rows = 24, std::size_t columns = 80,
                          int timeoutMilliseconds = 4000) {
    int readyPipe[2]{};
    int reportPipe[2]{};
    if (::pipe(readyPipe) == -1 || ::pipe(reportPipe) == -1) {
        throw std::system_error(errno, std::generic_category(), "pipe");
    }

    winsize size{};
    size.ws_row = static_cast<unsigned short>(rows);
    size.ws_col = static_cast<unsigned short>(columns);
    int master = -1;
    const auto childProcess = ::forkpty(&master, nullptr, nullptr, &size);
    if (childProcess == -1) {
        throw std::system_error(errno, std::generic_category(), "forkpty");
    }

    if (childProcess == 0) {
        ::close(readyPipe[0]);
        ::close(reportPipe[0]);
        try {
            child(readyPipe[1]);
            ::close(readyPipe[1]);
            ::close(reportPipe[1]);
            ::_exit(0);
        } catch (const std::exception& error) {
            const std::string report = error.what();
            static_cast<void>(::write(reportPipe[1], report.data(), report.size()));
        } catch (...) {
            constexpr std::string_view report = "unknown child exception";
            static_cast<void>(::write(reportPipe[1], report.data(), report.size()));
        }
        ::close(readyPipe[1]);
        ::close(reportPipe[1]);
        ::_exit(1);
    }

    ::close(readyPipe[1]);
    ::close(reportPipe[1]);
    makeNonBlocking(master);
    makeNonBlocking(readyPipe[0]);
    makeNonBlocking(reportPipe[0]);

    PtyResult result;
    int status = 0;
    bool exited = false;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMilliseconds);

    while (!result.becameReady && std::chrono::steady_clock::now() < deadline) {
        char marker = 0;
        const auto readResult = ::read(readyPipe[0], &marker, 1);
        if (readResult == 1) {
            result.becameReady = marker == 'R';
            break;
        }
        if (readResult == 0) {
            break;
        }
        appendAvailable(master, result.output);
        const auto waitResult = ::waitpid(childProcess, &status, WNOHANG);
        if (waitResult == childProcess) {
            exited = true;
            break;
        }
        pollfd descriptor{readyPipe[0], POLLIN | POLLHUP, 0};
        static_cast<void>(::poll(&descriptor, 1, 10));
    }

    auto pumpFor = [&](int milliseconds) {
        const auto pumpDeadline = std::min(
            deadline, std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(milliseconds));
        while (!exited && std::chrono::steady_clock::now() < pumpDeadline) {
            pollfd descriptor{master, POLLIN | POLLHUP, 0};
            static_cast<void>(::poll(&descriptor, 1, 10));
            appendAvailable(master, result.output);
            const auto waitResult = ::waitpid(childProcess, &status, WNOHANG);
            if (waitResult == childProcess) {
                exited = true;
            }
        }
    };

    if (result.becameReady && !exited) {
        for (const auto& chunk : input) {
            try {
                writeAll(master, chunk.bytes);
            } catch (const std::system_error&) {
                break;
            }
            if (chunk.pauseAfterMilliseconds > 0) {
                pumpFor(chunk.pauseAfterMilliseconds);
            }
            if (exited) {
                break;
            }
        }
    }

    while (!exited && std::chrono::steady_clock::now() < deadline) {
        pumpFor(25);
    }

    if (!exited) {
        result.timedOut = true;
        static_cast<void>(::kill(childProcess, SIGKILL));
        static_cast<void>(::waitpid(childProcess, &status, 0));
        exited = true;
    }

    appendAvailable(master, result.output);
    appendAvailable(reportPipe[0], result.report);

    if (WIFEXITED(status)) {
        result.exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.signal = WTERMSIG(status);
    }

    ::close(master);
    ::close(readyPipe[0]);
    ::close(reportPipe[0]);
    return result;
}

inline void checkPtySuccess(const PtyResult& result, const char* file, int line) {
    if (!result.becameReady) {
        fail("PTY child became ready", file, line, result.report);
    }
    if (result.timedOut) {
        fail("PTY child completed before timeout", file, line);
    }
    if (result.signal != 0) {
        fail("PTY child was not terminated by a signal", file, line,
             "signal " + std::to_string(result.signal));
    }
    if (result.exitCode != 0) {
        fail("PTY child exited successfully", file, line,
             result.report.empty() ? "exit " + std::to_string(result.exitCode)
                                   : result.report);
    }
}

} // namespace test

#define CHECK_PTY_SUCCESS(result)                                               \
    ::test::checkPtySuccess((result), __FILE__, __LINE__)

#endif // MINIVIM_PTY_SUPPORT_HPP
