#include "Renderer.hpp"

#include "TestSupport.hpp"

#include <string>
#include <vector>

namespace {

sjtu::Buffer lines(std::initializer_list<std::string> values,
                   const std::string& path = {}) {
    return sjtu::Buffer(std::vector<std::string>(values), path);
}

sjtu::Window windowFor(const sjtu::Buffer& buffer, sjtu::ScreenSize size,
                       sjtu::Position cursor = {}) {
    sjtu::Window window;
    window.resize(size);
    window.setNormalCursor(buffer, cursor);
    return window;
}

void frameOwnsCursorVisibilityHomeAndFinalPosition() {
    const auto buffer = lines({"abc"});
    const auto window = windowFor(buffer, {5, 40}, {0, 1});
    const sjtu::Renderer renderer;
    const auto frame = renderer.render(buffer, window, {});

    CHECK_EQ(frame.substr(0, 9), "\x1b[?25l\x1b[H");
    CHECK_CONTAINS(frame, "\x1b[1;2H");
    CHECK_EQ(frame.substr(frame.size() - 6), "\x1b[?25h");
}

void textRowsAreExpandedClearedAndFilledWithTildes() {
    const auto buffer = lines({"a\tb", std::string(1, '\x01')});
    const auto window = windowFor(buffer, {6, 20});
    const sjtu::Renderer renderer;
    const auto frame = renderer.render(buffer, window, {});

    CHECK_CONTAINS(frame, "a   b\x1b[K\r\n");
    CHECK_CONTAINS(frame, "^A\x1b[K\r\n");
    CHECK_CONTAINS(frame, "~\x1b[K\r\n~\x1b[K\r\n");
}

void statusLineShowsEveryModeNameFileDirtyFlagAndPosition() {
    auto buffer = lines({"alpha", "beta"}, "sample.txt");
    auto window = windowFor(buffer, {6, 60}, {1, 2});
    const sjtu::Renderer renderer;

    auto normal = renderer.render(buffer, window, {sjtu::Mode::Normal, {}, {}, {}});
    CHECK_CONTAINS(normal, " NORMAL  sample.txt");
    CHECK_CONTAINS(normal, "2,3 ");
    CHECK(normal.find("[+]") == std::string::npos);

    buffer.insertCharacter(0, 5, '!');
    auto insert = renderer.render(buffer, window, {sjtu::Mode::Insert, {}, {}, {}});
    CHECK_CONTAINS(insert, " INSERT  sample.txt [+]");

    auto command = renderer.render(buffer, window, {sjtu::Mode::CommandLine, {}, {}, {}});
    CHECK_CONTAINS(command, " COMMAND  sample.txt [+]");
}

void unnamedAndControlByteNamesAreRenderedSafely() {
    const auto unnamed = lines({""});
    const auto unnamedWindow = windowFor(unnamed, {4, 40});
    const sjtu::Renderer renderer;
    CHECK_CONTAINS(renderer.render(unnamed, unnamedWindow, {}), "[No Name]");

    const std::string unusualName = std::string("a") + '\x01' + ".txt";
    const auto named = lines({""}, unusualName);
    const auto namedWindow = windowFor(named, {4, 40});
    CHECK_CONTAINS(renderer.render(named, namedWindow, {}), "a^A.txt");
}

void normalBottomLineFitsMessageOnLeftAndPendingKeysOnRight() {
    const auto buffer = lines({"abc"});
    const auto window = windowFor(buffer, {5, 20});
    const sjtu::Renderer renderer;
    const auto frame = renderer.render(
        buffer, window, {sjtu::Mode::Normal, {}, "saved", "12d"});

    CHECK_CONTAINS(frame, "saved            12d\x1b[K");
}

void commandLineOverridesMessagesAndPlacesItsOwnCursor() {
    const auto buffer = lines({"abc"});
    const auto window = windowFor(buffer, {5, 20});
    const sjtu::Renderer renderer;
    const auto frame = renderer.render(
        buffer, window,
        {sjtu::Mode::CommandLine, "write", "hidden", "99"});

    CHECK_CONTAINS(frame, ":write              \x1b[K");
    CHECK(frame.find("hidden") == std::string::npos);
    CHECK(frame.find("99") == std::string::npos);
    CHECK_CONTAINS(frame, "\x1b[5;7H");
}

void horizontalViewportSlicesExpandedText() {
    const auto buffer = lines({"abcdefghij"});
    auto window = windowFor(buffer, {4, 4}, {0, 8});
    CHECK_EQ(window.viewport().left, 5U);
    const sjtu::Renderer renderer;
    const auto frame = renderer.render(buffer, window, {});
    CHECK_CONTAINS(frame, "fghi\x1b[K\r\n");
    CHECK_CONTAINS(frame, "\x1b[1;4H");

    const auto expanded = lines({"a\tb12345"});
    auto expandedWindow = windowFor(expanded, {4, 4}, {0, 6});
    const auto expandedFrame = renderer.render(expanded, expandedWindow, {});
    CHECK_CONTAINS(expandedFrame, "1234\x1b[K\r\n");
}

void narrowScreensClipStatusBottomLineAndCommandCursor() {
    auto buffer = lines({"abc"}, "a-very-long-name.txt");
    auto window = windowFor(buffer, {0, 0}, {0, 2});
    const sjtu::Renderer renderer;

    const auto normal = renderer.render(
        buffer, window, {sjtu::Mode::Normal, {}, "long message", "123"});
    CHECK_CONTAINS(normal, "3\x1b[K");
    CHECK_CONTAINS(normal, "\x1b[1;1H");

    const auto command = renderer.render(
        buffer, window, {sjtu::Mode::CommandLine, "abcdef", {}, {}});
    CHECK_CONTAINS(command, "\x1b[3;1H");
}

void renderingIsPureForTheSameSnapshot() {
    const auto buffer = lines({"one", "two"}, "sample.txt");
    const auto window = windowFor(buffer, {5, 30}, {1, 1});
    const sjtu::Renderer renderer;
    const sjtu::RenderState state{sjtu::Mode::Normal, {}, "message", "g"};
    const auto first = renderer.render(buffer, window, state);
    const auto second = renderer.render(buffer, window, state);
    CHECK_EQ(first, second);
    CHECK_EQ(window.cursor().row, 1U);
    CHECK_EQ(window.cursor().column, 1U);
    CHECK(!buffer.isModified());
}

} // namespace

int main() {
    return test::run({
        {"frame owns cursor visibility and positioning", frameOwnsCursorVisibilityHomeAndFinalPosition},
        {"text rows expand controls and fill unused rows", textRowsAreExpandedClearedAndFilledWithTildes},
        {"status line shows modes, file, dirty flag, and position", statusLineShowsEveryModeNameFileDirtyFlagAndPosition},
        {"unnamed and control-byte names render safely", unnamedAndControlByteNamesAreRenderedSafely},
        {"normal bottom line fits message and pending keys", normalBottomLineFitsMessageOnLeftAndPendingKeysOnRight},
        {"command line overrides messages and positions cursor", commandLineOverridesMessagesAndPlacesItsOwnCursor},
        {"horizontal viewport slices expanded text", horizontalViewportSlicesExpandedText},
        {"narrow screens clip all one-line UI elements", narrowScreensClipStatusBottomLineAndCommandCursor},
        {"rendering is pure for an unchanged snapshot", renderingIsPureForTheSameSnapshot},
    });
}
