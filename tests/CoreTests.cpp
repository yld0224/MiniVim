#include "Buffer.hpp"
#include "Command.hpp"
#include "Key.hpp"
#include "Renderer.hpp"
#include "TextLayout.hpp"
#include "Types.hpp"
#include "Window.hpp"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

sjtu::KeyEvent character(char value) {
    return sjtu::KeyEvent::character(static_cast<unsigned char>(value));
}

void testBufferAndTextLayout() {
    const sjtu::Buffer empty;
    assert(empty.lineCount() == 1);
    assert(empty.line(0).empty());
    assert(empty.displayName() == "[No Name]");

    const sjtu::Buffer buffer(
        std::vector<std::string>{"a\tb", std::string(1, '\x1b')},
        "sample.txt");
    assert(buffer.lineCount() == 2);
    assert(buffer.displayName() == "sample.txt");
    assert(sjtu::text::expandForDisplay(buffer.line(0)) == "a   b");
    assert(sjtu::text::expandForDisplay(buffer.line(1)) == "^[");
    assert(sjtu::text::screenColumn(buffer.line(0), 2) == 4);
    assert(sjtu::text::bufferColumn(buffer.line(0), 2) == 1);
}

void testNormalCommandParser() {
    sjtu::NormalCommandParser parser;

    assert(parser.feed(character('1')).kind == sjtu::ActionKind::None);
    assert(parser.feed(character('2')).kind == sjtu::ActionKind::None);
    assert(parser.pendingDisplay() == "12");
    const auto down = parser.feed(character('j'));
    assert(down.kind == sjtu::ActionKind::Move);
    assert(down.motion == sjtu::Motion::Down);
    assert(down.count == std::optional<std::size_t>{12});
    assert(parser.pendingDisplay().empty());

    assert(parser.feed(character('g')).kind == sjtu::ActionKind::None);
    assert(parser.pendingDisplay() == "g");
    const auto firstLine = parser.feed(character('g'));
    assert(firstLine.kind == sjtu::ActionKind::Move);
    assert(firstLine.motion == sjtu::Motion::FileStart);
    assert(!firstLine.count.has_value());

    const auto commandLine = parser.feed(character(':'));
    assert(commandLine.kind == sjtu::ActionKind::EnterCommandLine);

    const auto quit =
        parser.feed(sjtu::KeyEvent::character(sjtu::controlKey('q')));
    assert(quit.kind == sjtu::ActionKind::Quit);
}

void testVimStyleWindowMotion() {
    const sjtu::Buffer buffer(
        std::vector<std::string>{"abcdef", "\t", "123456"});
    sjtu::Window window;
    window.resize({8, 10});

    window.applyMotion(buffer, sjtu::Motion::Right, 4);
    assert(window.cursor().row == 0);
    assert(window.cursor().column == 4);

    // Vertical movement keeps the desired screen column across short lines.
    window.applyMotion(buffer, sjtu::Motion::Down);
    assert(window.cursor().row == 1);
    assert(window.cursor().column == 0);
    window.applyMotion(buffer, sjtu::Motion::Down);
    assert(window.cursor().row == 2);
    assert(window.cursor().column == 4);

    // Normal-mode left/right movement stops at line boundaries.
    window.applyMotion(buffer, sjtu::Motion::Right, 100);
    assert(window.cursor().row == 2);
    assert(window.cursor().column == 5);
    window.applyMotion(buffer, sjtu::Motion::Right);
    assert(window.cursor().row == 2);
    assert(window.cursor().column == 5);

    window.applyMotion(buffer, sjtu::Motion::FileStart);
    assert(window.cursor().row == 0);
    assert(window.cursor().column == 0);
    window.applyMotion(buffer, sjtu::Motion::FileEnd);
    assert(window.cursor().row == 2);
    assert(window.cursor().column == 0);
}

void testViewportAndRenderer() {
    const sjtu::Buffer buffer(
        std::vector<std::string>{"zero", "one", "two", "three", "four",
                                 "five", "six"},
        "sample.txt");
    sjtu::Window window;
    window.resize({5, 24});
    window.applyMotion(buffer, sjtu::Motion::Down, 5);
    assert(window.cursor().row == 5);
    assert(window.viewport().top == 3);

    const sjtu::Renderer renderer;
    const auto frame = renderer.render(
        buffer, window,
        {sjtu::Mode::Normal, "", "", ""});
    assert(frame.find("NORMAL") != std::string::npos);
    assert(frame.find("sample.txt") != std::string::npos);
    assert(frame.find("\x1b[?25l") != std::string::npos);
    assert(frame.find("\x1b[?25h") != std::string::npos);
}

} // namespace

int main() {
    testBufferAndTextLayout();
    testNormalCommandParser();
    testVimStyleWindowMotion();
    testViewportAndRenderer();
    std::cout << "Core tests passed\n";
}
