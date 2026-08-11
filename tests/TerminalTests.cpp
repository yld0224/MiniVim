#include "Terminal.hpp"

#include "PtySupport.hpp"
#include "TestSupport.hpp"

#include <cerrno>
#include <cstring>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace {

void checkKey(const sjtu::KeyEvent& actual, sjtu::KeyCode code,
              unsigned char value = 0) {
    CHECK_EQ(actual.code, code);
    CHECK_EQ(actual.value, value);
}

void ownershipTypeCannotBeCopiedOrMoved() {
    static_assert(!std::is_copy_constructible_v<sjtu::Terminal>);
    static_assert(!std::is_copy_assignable_v<sjtu::Terminal>);
    static_assert(!std::is_move_constructible_v<sjtu::Terminal>);
    static_assert(!std::is_move_assignable_v<sjtu::Terminal>);
    static_assert(std::is_nothrow_destructible_v<sjtu::Terminal>);
}

void constructorEnablesRawModeAndDestructorRestoresIt() {
    const auto result = test::runInPty([](int ready) {
        termios before{};
        CHECK_EQ(::tcgetattr(STDIN_FILENO, &before), 0);
        {
            sjtu::Terminal terminal;
            termios raw{};
            CHECK_EQ(::tcgetattr(STDIN_FILENO, &raw), 0);
            CHECK_EQ(raw.c_lflag & static_cast<tcflag_t>(ECHO | ICANON | IEXTEN | ISIG), 0U);
            CHECK_EQ(raw.c_iflag & static_cast<tcflag_t>(BRKINT | ICRNL | INPCK | ISTRIP | IXON), 0U);
            CHECK_EQ(raw.c_oflag & static_cast<tcflag_t>(OPOST), 0U);
            CHECK_NE(raw.c_cflag & static_cast<tcflag_t>(CS8), 0U);
            CHECK_EQ(raw.c_cc[VMIN], 0U);
            CHECK_EQ(raw.c_cc[VTIME], 1U);
            test::signalReady(ready);
        }
        termios after{};
        CHECK_EQ(::tcgetattr(STDIN_FILENO, &after), 0);
        CHECK_EQ(after.c_iflag, before.c_iflag);
        CHECK_EQ(after.c_oflag, before.c_oflag);
        CHECK_EQ(after.c_cflag, before.c_cflag);
        CHECK_EQ(after.c_lflag, before.c_lflag);
        for (std::size_t index = 0; index < NCCS; ++index) {
            CHECK_EQ(after.c_cc[index], before.c_cc[index]);
        }
    });
    CHECK_PTY_SUCCESS(result);
}

void readKeyDecodesCharactersEnterAndBackspace() {
    std::string bytes;
    bytes.push_back('x');
    bytes.push_back(static_cast<char>(0xFFU));
    bytes += "\r\n";
    bytes.push_back('\x7f');
    bytes.push_back('\b');

    const auto result = test::runInPty(
        [](int ready) {
            sjtu::Terminal terminal;
            test::signalReady(ready);
            checkKey(terminal.readKey(), sjtu::KeyCode::Character, 'x');
            checkKey(terminal.readKey(), sjtu::KeyCode::Character, 0xFFU);
            checkKey(terminal.readKey(), sjtu::KeyCode::Enter);
            checkKey(terminal.readKey(), sjtu::KeyCode::Enter);
            checkKey(terminal.readKey(), sjtu::KeyCode::Backspace);
            checkKey(terminal.readKey(), sjtu::KeyCode::Backspace);
        },
        {{bytes, 0}});
    CHECK_PTY_SUCCESS(result);
}

void readKeyDecodesCsiAndSs3NavigationSequences() {
    struct Expected {
        std::string bytes;
        sjtu::KeyCode code;
    };
    const std::vector<Expected> expected{
        {"\x1b[A", sjtu::KeyCode::ArrowUp},
        {"\x1b[B", sjtu::KeyCode::ArrowDown},
        {"\x1b[C", sjtu::KeyCode::ArrowRight},
        {"\x1b[D", sjtu::KeyCode::ArrowLeft},
        {"\x1b[H", sjtu::KeyCode::Home},
        {"\x1b[F", sjtu::KeyCode::End},
        {"\x1bOH", sjtu::KeyCode::Home},
        {"\x1bOF", sjtu::KeyCode::End},
        {"\x1b[1~", sjtu::KeyCode::Home},
        {"\x1b[7~", sjtu::KeyCode::Home},
        {"\x1b[3~", sjtu::KeyCode::Delete},
        {"\x1b[4~", sjtu::KeyCode::End},
        {"\x1b[8~", sjtu::KeyCode::End},
        {"\x1b[5~", sjtu::KeyCode::PageUp},
        {"\x1b[6~", sjtu::KeyCode::PageDown},
    };
    std::string input;
    for (const auto& item : expected) {
        input += item.bytes;
    }

    const auto result = test::runInPty(
        [expected](int ready) {
            sjtu::Terminal terminal;
            test::signalReady(ready);
            for (const auto& item : expected) {
                checkKey(terminal.readKey(), item.code);
            }
        },
        {{input, 0}});
    CHECK_PTY_SUCCESS(result);
}

void escapeAndUnknownOrPartialSequencesBecomeEscape() {
    const auto isolated = test::runInPty(
        [](int ready) {
            sjtu::Terminal terminal;
            test::signalReady(ready);
            checkKey(terminal.readKey(), sjtu::KeyCode::Escape);
        },
        {{"\x1b", 0}});
    CHECK_PTY_SUCCESS(isolated);

    const std::vector<std::string> unknownSequences{
        "\x1b[9~", "\x1b[Z", "\x1bOX", "\x1b[2x", "\x1b[",
    };
    for (const auto& sequence : unknownSequences) {
        const auto result = test::runInPty(
            [](int ready) {
                sjtu::Terminal terminal;
                test::signalReady(ready);
                checkKey(terminal.readKey(), sjtu::KeyCode::Escape);
            },
            {{sequence, 0}});
        CHECK_PTY_SUCCESS(result);
    }
}

void screenSizeUsesTheTerminalIoctlWhenAvailable() {
    const auto result = test::runInPty(
        [](int ready) {
            sjtu::Terminal terminal;
            test::signalReady(ready);
            const auto size = terminal.screenSize();
            CHECK_EQ(size.rows, 37U);
            CHECK_EQ(size.columns, 101U);
        },
        {}, 37, 101);
    CHECK_PTY_SUCCESS(result);
}

void screenSizeFallsBackToCursorPositionQuery() {
    const auto result = test::runInPty(
        [](int ready) {
            sjtu::Terminal terminal;
            test::signalReady(ready);
            const auto size = terminal.screenSize();
            CHECK_EQ(size.rows, 24U);
            CHECK_EQ(size.columns, 80U);
        },
        {{"\x1b[24;80R", 0}}, 0, 0);
    CHECK_PTY_SUCCESS(result);
    CHECK_CONTAINS(result.output, "\x1b[999C\x1b[999B\x1b[6n");
}

void invalidCursorPositionResponseThrows() {
    const auto result = test::runInPty(
        [](int ready) {
            sjtu::Terminal terminal;
            test::signalReady(ready);
            const auto message = EXPECT_THROWS(std::runtime_error, terminal.screenSize());
            CHECK_EQ(message, "cannot determine terminal size");
        },
        {{"not-a-response", 0}}, 0, 0);
    CHECK_PTY_SUCCESS(result);
}

void outputAndClearScreenWriteExactBytes() {
    const auto result = test::runInPty([](int ready) {
        sjtu::Terminal terminal;
        test::signalReady(ready);
        const std::string binary("alpha\0beta", 10);
        terminal.writeOutput(binary);
        terminal.clearScreen();
    });
    CHECK_PTY_SUCCESS(result);
    CHECK_EQ(result.output, std::string("alpha\0beta\x1b[2J\x1b[H", 17));
}

void outputFailuresAreReportedAsSystemErrors() {
    const auto result = test::runInPty([](int ready) {
        sjtu::Terminal terminal;
        test::signalReady(ready);
        CHECK_EQ(::close(STDOUT_FILENO), 0);
        const auto message = EXPECT_THROWS(std::system_error,
                                           terminal.writeOutput("x"));
        CHECK_CONTAINS(message, "write");
    });
    CHECK_PTY_SUCCESS(result);
}

void constructionWithoutATtyFailsCleanly() {
    const auto child = ::fork();
    CHECK_NE(child, -1);
    if (child == 0) {
        int input[2]{};
        if (::pipe(input) == -1) {
            ::_exit(2);
        }
        if (::dup2(input[0], STDIN_FILENO) == -1) {
            ::_exit(3);
        }
        ::close(input[0]);
        ::close(input[1]);
        try {
            sjtu::Terminal terminal;
        } catch (const std::system_error& error) {
            ::_exit(std::string(error.what()).find("tcgetattr") != std::string::npos ? 0 : 4);
        } catch (...) {
            ::_exit(5);
        }
        ::_exit(6);
    }

    int status = 0;
    CHECK_EQ(::waitpid(child, &status, 0), child);
    CHECK(WIFEXITED(status));
    CHECK_EQ(WEXITSTATUS(status), 0);
}

} // namespace

int main() {
    return test::run({
        {"Terminal cannot be copied or moved", ownershipTypeCannotBeCopiedOrMoved},
        {"Terminal enables raw mode and restores it", constructorEnablesRawModeAndDestructorRestoresIt},
        {"readKey decodes characters, Enter, and Backspace", readKeyDecodesCharactersEnterAndBackspace},
        {"readKey decodes CSI and SS3 navigation", readKeyDecodesCsiAndSs3NavigationSequences},
        {"unknown and partial sequences become Escape", escapeAndUnknownOrPartialSequencesBecomeEscape},
        {"screenSize uses TIOCGWINSZ", screenSizeUsesTheTerminalIoctlWhenAvailable},
        {"screenSize falls back to a cursor query", screenSizeFallsBackToCursorPositionQuery},
        {"invalid cursor response throws", invalidCursorPositionResponseThrows},
        {"writeOutput and clearScreen write exact bytes", outputAndClearScreenWriteExactBytes},
        {"writeOutput reports system errors", outputFailuresAreReportedAsSystemErrors},
        {"construction without a TTY fails cleanly", constructionWithoutATtyFailsCleanly},
    });
}
