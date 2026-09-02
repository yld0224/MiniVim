/*
Basic部分 模块测试
这是Window相关的测试,它只测试你是否正确实现了Window模块的公共接口约定.
Note: 如果你的vscode里面的c++ intellisense/clangd报错了,是正常的,因为它没有正确检测到文件的依赖关系,这对于我们的make test指令没有影响.
*/
#include "Window.hpp"

#include "TestSupport.hpp"

#include <string>
#include <vector>

namespace {

sjtu::Buffer Lines(std::initializer_list<std::string> values) {
    return sjtu::Buffer(std::vector<std::string>(values));
}

void CheckPosition(const sjtu::Window& window, std::size_t row,
                   std::size_t column) {
    CHECK_EQ(window.GetCursor().row_, row);
    CHECK_EQ(window.GetCursor().column_, column);
}

void ResizeReservesOneMessageOrCommandRow() {
    sjtu::Window window;
    window.Resize({24, 80});
    CHECK_EQ(window.GetViewport().rows_, 23U);
    CHECK_EQ(window.GetViewport().columns_, 80U);

    window.Resize({2, 0});
    CHECK_EQ(window.GetViewport().rows_, 1U);
    CHECK_EQ(window.GetViewport().columns_, 1U);
}

void SetCursorDistinguishesNormalAndInsertBounds() {
    const auto buffer = Lines({"abc", ""});
    sjtu::Window window;
    window.Resize({6, 20});

    window.SetCursor(buffer, {0, 99}, false);
    CheckPosition(window, 0, 2);
    window.SetCursor(buffer, {0, 99}, true);
    CheckPosition(window, 0, 3);

    window.SetCursor(buffer, {99, 99}, false);
    CheckPosition(window, 1, 0);
    window.SetCursor(buffer, {99, 99}, true);
    CheckPosition(window, 1, 0);
}

void HorizontalMotionsStopAtNormalModeLineBounds() {
    const auto buffer = Lines({"abc", ""});
    sjtu::Window window;
    window.Resize({6, 20});
    window.SetCursor(buffer, {0, 1}, false);

    window.ApplyMotion(buffer, sjtu::Motion::Left);
    CheckPosition(window, 0, 0);
    window.ApplyMotion(buffer, sjtu::Motion::Left);
    CheckPosition(window, 0, 0);

    window.ApplyMotion(buffer, sjtu::Motion::Right);
    window.ApplyMotion(buffer, sjtu::Motion::Right);
    window.ApplyMotion(buffer, sjtu::Motion::Right);
    CheckPosition(window, 0, 2);

    window.SetCursor(buffer, {1, 0}, false);
    window.ApplyMotion(buffer, sjtu::Motion::Left);
    window.ApplyMotion(buffer, sjtu::Motion::Right);
    CheckPosition(window, 1, 0);
}

void VerticalMotionsClampAndRememberTheDesiredColumn() {
    const auto buffer = Lines({"abcdef", "x", "123456"});
    sjtu::Window window;
    window.Resize({8, 20});
    window.SetCursor(buffer, {0, 4}, false);

    window.ApplyMotion(buffer, sjtu::Motion::Down);
    CheckPosition(window, 1, 0);
    window.ApplyMotion(buffer, sjtu::Motion::Down);
    CheckPosition(window, 2, 4);
    window.ApplyMotion(buffer, sjtu::Motion::Down);
    CheckPosition(window, 2, 4);
    window.ApplyMotion(buffer, sjtu::Motion::Up);
    CheckPosition(window, 1, 0);
    window.ApplyMotion(buffer, sjtu::Motion::Up);
    CheckPosition(window, 0, 4);
    window.ApplyMotion(buffer, sjtu::Motion::Up);
    CheckPosition(window, 0, 4);
}

void DesiredColumnUsesRenderedColumnsForTabs() {
    const auto buffer = Lines({"abcd", "\t", "12345"});
    sjtu::Window window;
    window.Resize({8, 20});
    window.SetCursor(buffer, {0, 3}, false);

    window.ApplyMotion(buffer, sjtu::Motion::Down);
    CheckPosition(window, 1, 0);
    window.ApplyMotion(buffer, sjtu::Motion::Down);
    CheckPosition(window, 2, 3);
}

void EnsureCursorVisibleTracksVerticalAndHorizontalOffsets() {
    std::vector<std::string> contents(10, "0123456789");
    const sjtu::Buffer buffer(std::move(contents));
    sjtu::Window window;
    window.Resize({5, 4});

    window.SetCursor(buffer, {6, 8}, false);
    const auto visible_rows = window.GetViewport().rows_;
    CHECK_EQ(window.GetViewport().top_, 6U - visible_rows + 1U);
    CHECK_EQ(window.GetViewport().left_, 5U);

    window.SetCursor(buffer, {1, 2}, false);
    CHECK_EQ(window.GetViewport().top_, 1U);
    CHECK_EQ(window.GetViewport().left_, 2U);
}

} // namespace

int main() {
    return test::Run({
        {"Resize reserves one message or command row", ResizeReservesOneMessageOrCommandRow},
        {"SetCursor distinguishes Normal and Insert bounds", SetCursorDistinguishesNormalAndInsertBounds},
        {"horizontal motions stop at Normal-mode bounds", HorizontalMotionsStopAtNormalModeLineBounds},
        {"vertical motions clamp and remember desired column", VerticalMotionsClampAndRememberTheDesiredColumn},
        {"desired column is measured in rendered columns", DesiredColumnUsesRenderedColumnsForTabs},
        {"EnsureCursorVisible updates both viewport offsets", EnsureCursorVisibleTracksVerticalAndHorizontalOffsets},
    });
}
