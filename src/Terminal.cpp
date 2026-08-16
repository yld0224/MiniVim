#include "Terminal.hpp"

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <system_error>

#include <sys/ioctl.h>
#include <unistd.h>

namespace sjtu {
namespace {

void ThrowSystemError(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}

} 

Terminal::Terminal() {

    if (::tcgetattr(STDIN_FILENO, &original_) == -1) {
        ThrowSystemError("tcgetattr");
    }

    termios raw = original_;
    tcflag_t inputFlags = BRKINT | ICRNL | INPCK | ISTRIP | IXON;
    tcflag_t outputFlags = OPOST;
    tcflag_t localFlags = ECHO | ICANON | IEXTEN | ISIG;

    raw.c_iflag &= ~inputFlags;
    raw.c_oflag &= ~outputFlags;
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~localFlags;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        ThrowSystemError("tcsetattr");
    }
    raw_mode_enabled_ = true;
}

Terminal::~Terminal() noexcept {
    if (raw_mode_enabled_) {
        ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
    }
}

KeyEvent Terminal::ReadKey() {
    auto first = ReadByte();
    switch (first) {
    case '\r':
    case '\n':
        return {KeyCode::Enter, 0};
    case 0x7FU:
    case '\b':
        return {KeyCode::Backspace, 0};
    case 0x1BU:
        break;
    default:
        return KeyEvent::Character(first);
    }

    auto second = TryReadByte();
    if (!second.has_value()) {
        return {KeyCode::Escape, 0};
    }
    auto third = TryReadByte();
    if (!third.has_value()) {
        return {KeyCode::Escape, 0};
    }

    if (*second == '[') {
        if (*third >= '0' && *third <= '9') {
            auto fourth = TryReadByte();
            if (!fourth.has_value() || *fourth != '~') {
                return {KeyCode::Escape, 0};
            }
            switch (*third) {
            case '1':
            case '7':
                return {KeyCode::Home, 0};
            case '3':
                return {KeyCode::Delete, 0};
            case '4':
            case '8':
                return {KeyCode::End, 0};
            case '5':
                return {KeyCode::PageUp, 0};
            case '6':
                return {KeyCode::PageDown, 0};
            default:
                return {KeyCode::Escape, 0};
            }
        }

        switch (*third) {
        case 'A':
            return {KeyCode::ArrowUp, 0};
        case 'B':
            return {KeyCode::ArrowDown, 0};
        case 'C':
            return {KeyCode::ArrowRight, 0};
        case 'D':
            return {KeyCode::ArrowLeft, 0};
        case 'H':
            return {KeyCode::Home, 0};
        case 'F':
            return {KeyCode::End, 0};
        default:
            return {KeyCode::Escape, 0};
        }
    }

    if (*second == 'O') {
        if (*third == 'H') {
            return {KeyCode::Home, 0};
        }
        if (*third == 'F') {
            return {KeyCode::End, 0};
        }
    }
    return {KeyCode::Escape, 0};
}

ScreenSize Terminal::GetScreenSize() {
    winsize size{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_row > 0 && size.ws_col > 0) {
        return {static_cast<std::size_t>(size.ws_row), static_cast<std::size_t>(size.ws_col)};
    }

    WriteOutput("\x1b[999C\x1b[999B");
    return QueryCursorPosition();
}

void Terminal::WriteOutput(std::string_view output) {
    std::size_t written = 0;
    while (written < output.size()) {
        auto result = ::write(STDOUT_FILENO, output.data() + written, output.size() - written);
        if (result > 0) {
            written += static_cast<std::size_t>(result);
            continue;
        }
        if (result == -1 && errno == EINTR) { continue; }
        ThrowSystemError("write");
    }
}

void Terminal::ClearScreen() {
    WriteOutput("\x1b[2J\x1b[H");
}

unsigned char Terminal::ReadByte() {
    while (true) {
        unsigned char value = 0;
        auto result = ::read(STDIN_FILENO, &value, 1);
        if (result == 1) { return value; }
        if (result == -1 && errno != EAGAIN && errno != EINTR) { ThrowSystemError("read"); }
    }
}

std::optional<unsigned char> Terminal::TryReadByte() {
    while (true) {
        unsigned char value = 0;
        auto result = ::read(STDIN_FILENO, &value, 1);
        if (result == 1) { return value; }
        if (result == 0 || (result == -1 && errno == EAGAIN)) { return std::nullopt; }
        if (result == -1 && errno == EINTR) { continue; }
        ThrowSystemError("read");
    }
}

ScreenSize Terminal::QueryCursorPosition() {
    WriteOutput("\x1b[6n");

    char response[32]{};
    std::size_t length = 0;
    while (length + 1 < sizeof(response)) {
        auto byte = TryReadByte();
        if (!byte.has_value()) { break; }
        response[length] = static_cast<char>(*byte);
        if (response[length] == 'R') {
            ++length;
            break;
        }
        ++length;
    }
    response[length] = '\0';

    int rows = 0;
    int columns = 0;
    if (length < 4 || response[0] != '\x1b' || response[1] != '[' || std::sscanf(response + 2, "%d;%d", &rows, &columns) != 2
     || rows <= 0 || columns <= 0) {
        throw std::runtime_error("cannot determine terminal size");
    }
    return {static_cast<std::size_t>(rows), static_cast<std::size_t>(columns)};
}

} // namespace sjtu
