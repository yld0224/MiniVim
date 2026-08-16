#include "Renderer.hpp"

#include "TestSupport.hpp"

#include <string>
#include <vector>

namespace {

sjtu::Buffer Lines(std::initializer_list<std::string> values,
                   const std::string& path = {}) {
    return sjtu::Buffer(std::vector<std::string>(values), path);
}

sjtu::Window MakeWindow(const sjtu::Buffer& buffer, sjtu::ScreenSize size,
                        sjtu::Position cursor = {}) {
    sjtu::Window window;
    window.Resize(size);
    window.SetCursor(buffer, cursor, false);
    return window;
}

void FrameControlsCursorVisibilityAndPosition() {
    const auto buffer = Lines({"abc"});
    const auto window = MakeWindow(buffer, {5, 40}, {0, 1});
    const sjtu::Renderer renderer;
    const auto frame = renderer.Render(buffer, window, {});

    CHECK_EQ(frame.substr(0, 9), "\x1b[?25l\x1b[H");
    CHECK_CONTAINS(frame, "\x1b[1;2H");
    CHECK_EQ(frame.substr(frame.size() - 6), "\x1b[?25h");
}

void TextRowsExpandBytesAndMarkRowsPastTheBuffer() {
    const auto buffer = Lines({"a\tb", std::string(1, '\x01')});
    const auto window = MakeWindow(buffer, {6, 30});
    const sjtu::Renderer renderer;
    const auto frame = renderer.Render(buffer, window, {});

    CHECK_CONTAINS(frame, "a   b\x1b[K\r\n");
    CHECK_CONTAINS(frame, "^A\x1b[K\r\n");
    CHECK_CONTAINS(frame, "~\x1b[K\r\n");
}

void StatusShowsModeNameFileDirtyFlagAndPosition() {
    auto buffer = Lines({"alpha", "beta"}, "sample.txt");
    auto window = MakeWindow(buffer, {6, 60}, {1, 2});
    const sjtu::Renderer renderer;

    const auto normal = renderer.Render(
        buffer, window, {sjtu::Mode::Normal, {}, {}});
    CHECK_CONTAINS(normal, " NORMAL  sample.txt");
    CHECK_CONTAINS(normal, "2,3 ");
    CHECK(normal.find("[+]") == std::string::npos);

    buffer.InsertCharacter(0, 5, '!');
    const auto insert = renderer.Render(
        buffer, window, {sjtu::Mode::Insert, {}, {}});
    CHECK_CONTAINS(insert, " INSERT  sample.txt [+]");

    const auto command = renderer.Render(
        buffer, window, {sjtu::Mode::CommandLine, {}, {}});
    CHECK_CONTAINS(command, " COMMAND  sample.txt [+]");
}

void BottomAreaShowsMessagesOrTheCommandBuffer() {
    const auto buffer = Lines({"abc"});
    const auto window = MakeWindow(buffer, {5, 30});
    const sjtu::Renderer renderer;

    const auto normal = renderer.Render(
        buffer, window, {sjtu::Mode::Normal, {}, "saved"});
    CHECK_CONTAINS(normal, "saved");

    const auto command = renderer.Render(
        buffer, window,
        {sjtu::Mode::CommandLine, "wq output.txt", "hidden"});
    CHECK_CONTAINS(command, ":wq output.txt");
    CHECK(command.find("hidden") == std::string::npos);

    const auto cursor_row = window.GetViewport().rows_ + 2;
    const auto cursor_column = std::string("wq output.txt").size() + 2;
    CHECK_CONTAINS(command,
                   "\x1b[" + std::to_string(cursor_row) + ';' +
                       std::to_string(cursor_column) + 'H');
}

void HorizontalViewportSlicesExpandedText() {
    const auto buffer = Lines({"abcdefghij"});
    const auto window = MakeWindow(buffer, {4, 4}, {0, 8});
    CHECK_EQ(window.GetViewport().left_, 5U);
    const sjtu::Renderer renderer;
    const auto frame = renderer.Render(buffer, window, {});
    CHECK_CONTAINS(frame, "fghi\x1b[K\r\n");
    CHECK_CONTAINS(frame, "\x1b[1;4H");
}

void RenderingDoesNotMutateItsInputs() {
    const auto buffer = Lines({"one", "two"}, "sample.txt");
    const auto window = MakeWindow(buffer, {5, 30}, {1, 1});
    const sjtu::Renderer renderer;
    const sjtu::RenderState state{sjtu::Mode::Normal, {}, "message"};

    const auto first = renderer.Render(buffer, window, state);
    const auto second = renderer.Render(buffer, window, state);
    CHECK_EQ(first, second);
    CHECK_EQ(window.GetCursor().row_, 1U);
    CHECK_EQ(window.GetCursor().column_, 1U);
    CHECK(!buffer.IsModified());
}

} // namespace

int main() {
    return test::Run({
        {"frame controls cursor visibility and position", FrameControlsCursorVisibilityAndPosition},
        {"text rows expand bytes and mark rows past the Buffer", TextRowsExpandBytesAndMarkRowsPastTheBuffer},
        {"status shows mode, file, dirty flag, and position", StatusShowsModeNameFileDirtyFlagAndPosition},
        {"bottom area shows either messages or command input", BottomAreaShowsMessagesOrTheCommandBuffer},
        {"horizontal viewport slices expanded text", HorizontalViewportSlicesExpandedText},
        {"Render does not mutate its inputs", RenderingDoesNotMutateItsInputs},
    });
}
