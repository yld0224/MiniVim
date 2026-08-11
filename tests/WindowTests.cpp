#include "Window.hpp"

#include "TestSupport.hpp"

#include <limits>
#include <string>
#include <vector>

namespace {

sjtu::Buffer lines(std::initializer_list<std::string> values) {
    return sjtu::Buffer(std::vector<std::string>(values));
}

void checkPosition(const sjtu::Window& window, std::size_t row,
                   std::size_t column) {
    CHECK_EQ(window.cursor().row, row);
    CHECK_EQ(window.cursor().column, column);
}

sjtu::Buffer numberedLines(std::size_t count) {
    std::vector<std::string> contents;
    for (std::size_t row = 0; row < count; ++row) {
        contents.push_back("  line " + std::to_string(row));
    }
    return sjtu::Buffer(std::move(contents));
}

void resizeReservesTwoRowsAndClampsTinyScreens() {
    sjtu::Window window;
    window.resize({24, 80});
    CHECK_EQ(window.viewport().textRows, 22U);
    CHECK_EQ(window.viewport().columns, 80U);

    window.resize({2, 0});
    CHECK_EQ(window.viewport().textRows, 1U);
    CHECK_EQ(window.viewport().columns, 1U);

    window.resize({0, 0});
    CHECK_EQ(window.viewport().textRows, 1U);
    CHECK_EQ(window.viewport().columns, 1U);
}

void cursorSettersClampRowsAndRespectModeLineEnds() {
    const auto buffer = lines({"abc", ""});
    sjtu::Window window;
    window.resize({6, 20});

    window.setNormalCursor(buffer, {0, 99});
    checkPosition(window, 0, 2);
    window.setInsertCursor(buffer, {0, 99});
    checkPosition(window, 0, 3);

    window.setNormalCursor(buffer, {99, 99});
    checkPosition(window, 1, 0);
    window.setInsertCursor(buffer, {99, 99});
    checkPosition(window, 1, 0);
}

void cursorScreenColumnUsesExpandedText() {
    const auto buffer = lines({"a\tb\x01"});
    sjtu::Window window;
    window.resize({6, 20});
    window.setInsertCursor(buffer, {0, 2});
    CHECK_EQ(window.cursorScreenColumn(buffer), 4U);
    window.setInsertCursor(buffer, {0, 4});
    CHECK_EQ(window.cursorScreenColumn(buffer), 7U);
}

void ensuringVisibilityScrollsVerticallyAndHorizontally() {
    auto buffer = numberedLines(10);
    sjtu::Window window;
    window.resize({5, 4});
    CHECK_EQ(window.viewport().textRows, 3U);

    window.setNormalCursor(buffer, {5, 7});
    CHECK_EQ(window.viewport().top, 3U);
    CHECK_EQ(window.viewport().left, 4U);

    window.setNormalCursor(buffer, {2, 2});
    CHECK_EQ(window.viewport().top, 2U);
    CHECK_EQ(window.viewport().left, 2U);

    window.ensureCursorVisible(buffer);
    CHECK_EQ(window.viewport().top, 2U);
    CHECK_EQ(window.viewport().left, 2U);
}

void horizontalMotionsClampAndTreatZeroCountAsOne() {
    const auto buffer = lines({"abcde", ""});
    sjtu::Window window;
    window.resize({5, 20});
    window.setNormalCursor(buffer, {0, 2});

    window.applyMotion(buffer, sjtu::Motion::Left, 0U);
    checkPosition(window, 0, 1);
    window.applyMotion(buffer, sjtu::Motion::Left, 99U);
    checkPosition(window, 0, 0);
    window.applyMotion(buffer, sjtu::Motion::Right, 99U);
    checkPosition(window, 0, 4);
    window.applyMotion(buffer, sjtu::Motion::Right);
    checkPosition(window, 0, 4);

    window.setNormalCursor(buffer, {1, 0});
    window.applyMotion(buffer, sjtu::Motion::Left);
    window.applyMotion(buffer, sjtu::Motion::Right);
    checkPosition(window, 1, 0);
}

void verticalMotionsPreserveTheDesiredScreenColumn() {
    const auto buffer = lines({"abcdef", "\t", "123456", "x"});
    sjtu::Window window;
    window.resize({8, 20});
    window.setNormalCursor(buffer, {0, 4});

    window.applyMotion(buffer, sjtu::Motion::Down);
    checkPosition(window, 1, 0);
    window.applyMotion(buffer, sjtu::Motion::Down);
    checkPosition(window, 2, 4);
    window.applyMotion(buffer, sjtu::Motion::Down, 99U);
    checkPosition(window, 3, 0);
    window.applyMotion(buffer, sjtu::Motion::Up, 2U);
    checkPosition(window, 1, 0);
    window.applyMotion(buffer, sjtu::Motion::Up, 99U);
    checkPosition(window, 0, 4);
}

void lineEdgeMotionsHandleWhitespaceEmptyLinesAndCounts() {
    const auto buffer = lines({"  abc", "", "\txyz", "last"});
    sjtu::Window window;
    window.resize({8, 20});
    window.setNormalCursor(buffer, {0, 4});

    window.applyMotion(buffer, sjtu::Motion::LineStart);
    checkPosition(window, 0, 0);
    window.applyMotion(buffer, sjtu::Motion::FirstNonBlank);
    checkPosition(window, 0, 2);
    window.applyMotion(buffer, sjtu::Motion::LineEnd, 2U);
    checkPosition(window, 1, 0);
    window.applyMotion(buffer, sjtu::Motion::FirstNonBlank);
    checkPosition(window, 1, 0);

    window.setNormalCursor(buffer, {2, 0});
    window.applyMotion(buffer, sjtu::Motion::LineEnd);
    checkPosition(window, 2, 3);
}

void fileMotionsImplementGgAndGCountSemantics() {
    const auto buffer = lines({"  zero", " one", "\ttwo", "three"});
    sjtu::Window window;
    window.resize({8, 20});
    window.setNormalCursor(buffer, {3, 3});

    window.applyMotion(buffer, sjtu::Motion::FileStart);
    checkPosition(window, 0, 2);
    window.applyMotion(buffer, sjtu::Motion::FileStart, 3U);
    checkPosition(window, 2, 1);
    window.applyMotion(buffer, sjtu::Motion::FileEnd);
    checkPosition(window, 3, 0);
    window.applyMotion(buffer, sjtu::Motion::FileEnd, 2U);
    checkPosition(window, 1, 1);
    window.applyMotion(buffer, sjtu::Motion::FileEnd, 999U);
    checkPosition(window, 3, 0);
}

void windowRelativeMotionsUseTheVisibleRegion() {
    const auto buffer = numberedLines(10);
    sjtu::Window window;
    window.resize({5, 40});
    window.setNormalCursor(buffer, {5, 0});
    CHECK_EQ(window.viewport().top, 3U);

    window.applyMotion(buffer, sjtu::Motion::WindowTop);
    checkPosition(window, 3, 2);
    window.applyMotion(buffer, sjtu::Motion::WindowTop, 2U);
    checkPosition(window, 4, 2);
    window.applyMotion(buffer, sjtu::Motion::WindowMiddle);
    checkPosition(window, 4, 2);
    window.applyMotion(buffer, sjtu::Motion::WindowBottom);
    checkPosition(window, 5, 2);
    window.applyMotion(buffer, sjtu::Motion::WindowBottom, 2U);
    checkPosition(window, 4, 2);
    window.applyMotion(buffer, sjtu::Motion::WindowBottom, 99U);
    checkPosition(window, 3, 2);
}

void pageMotionsUseViewportHeightCountsAndSaturation() {
    const auto buffer = numberedLines(30);
    sjtu::Window window;
    window.resize({6, 40});
    window.setNormalCursor(buffer, {10, 4});
    CHECK_EQ(window.viewport().textRows, 4U);

    window.applyMotion(buffer, sjtu::Motion::PageUp);
    checkPosition(window, 6, 4);
    window.applyMotion(buffer, sjtu::Motion::HalfPageUp);
    checkPosition(window, 4, 4);
    window.applyMotion(buffer, sjtu::Motion::HalfPageDown, 2U);
    checkPosition(window, 8, 4);
    window.applyMotion(buffer, sjtu::Motion::PageDown, 2U);
    checkPosition(window, 16, 4);
    window.applyMotion(buffer, sjtu::Motion::PageDown,
                       std::numeric_limits<std::size_t>::max());
    checkPosition(window, 29, 4);
    window.applyMotion(buffer, sjtu::Motion::PageUp,
                       std::numeric_limits<std::size_t>::max());
    checkPosition(window, 0, 4);
}

void wordForwardDistinguishesWordsPunctuationAndLines() {
    const auto buffer = lines({"one_two ++ two", "  next", "", "last"});
    sjtu::Window window;
    window.resize({8, 40});

    window.applyMotion(buffer, sjtu::Motion::WordForward);
    checkPosition(window, 0, 8);
    window.applyMotion(buffer, sjtu::Motion::WordForward);
    checkPosition(window, 0, 11);
    window.applyMotion(buffer, sjtu::Motion::WordForward);
    checkPosition(window, 1, 2);
    window.applyMotion(buffer, sjtu::Motion::WordForward);
    checkPosition(window, 3, 0);
    window.applyMotion(buffer, sjtu::Motion::WordForward);
    checkPosition(window, 3, 3);
    window.applyMotion(buffer, sjtu::Motion::WordForward);
    checkPosition(window, 3, 3);

    window.setNormalCursor(buffer, {0, 0});
    window.applyMotion(buffer, sjtu::Motion::WordForward, 3U);
    checkPosition(window, 1, 2);
}

void wordBackwardSkipsWhitespaceAndCrossesLines() {
    const auto buffer = lines({"one ++ two", "  next item", "", "last"});
    sjtu::Window window;
    window.resize({8, 40});
    window.setNormalCursor(buffer, {1, 7});

    window.applyMotion(buffer, sjtu::Motion::WordBackward);
    checkPosition(window, 1, 2);
    window.applyMotion(buffer, sjtu::Motion::WordBackward);
    checkPosition(window, 0, 7);
    window.applyMotion(buffer, sjtu::Motion::WordBackward);
    checkPosition(window, 0, 4);
    window.applyMotion(buffer, sjtu::Motion::WordBackward);
    checkPosition(window, 0, 0);
    window.applyMotion(buffer, sjtu::Motion::WordBackward);
    checkPosition(window, 0, 0);

    window.setNormalCursor(buffer, {3, 2});
    window.applyMotion(buffer, sjtu::Motion::WordBackward, 2U);
    checkPosition(window, 1, 7);
}

void wordEndFindsClassEndsAndCrossesBlankLines() {
    const auto buffer = lines({"one_two ++ two", "", " next"});
    sjtu::Window window;
    window.resize({8, 40});

    window.applyMotion(buffer, sjtu::Motion::WordEnd);
    checkPosition(window, 0, 6);
    window.applyMotion(buffer, sjtu::Motion::WordEnd);
    checkPosition(window, 0, 9);
    window.applyMotion(buffer, sjtu::Motion::WordEnd);
    checkPosition(window, 0, 13);
    window.applyMotion(buffer, sjtu::Motion::WordEnd);
    checkPosition(window, 2, 4);
    window.applyMotion(buffer, sjtu::Motion::WordEnd);
    checkPosition(window, 2, 4);

    window.setNormalCursor(buffer, {0, 0});
    window.applyMotion(buffer, sjtu::Motion::WordEnd, 3U);
    checkPosition(window, 0, 13);
}

void aHorizontalMoveReplacesTheRememberedVerticalColumn() {
    const auto buffer = lines({"abcdefgh", "x", "abcdefgh"});
    sjtu::Window window;
    window.resize({8, 40});
    window.setNormalCursor(buffer, {0, 6});
    window.applyMotion(buffer, sjtu::Motion::Down);
    checkPosition(window, 1, 0);
    window.applyMotion(buffer, sjtu::Motion::Right);
    checkPosition(window, 1, 0);
    window.applyMotion(buffer, sjtu::Motion::Down);
    checkPosition(window, 2, 0);
}

} // namespace

int main() {
    return test::run({
        {"resize reserves status rows and clamps tiny screens", resizeReservesTwoRowsAndClampsTinyScreens},
        {"cursor setters clamp rows and respect mode line ends", cursorSettersClampRowsAndRespectModeLineEnds},
        {"cursorScreenColumn uses expanded text", cursorScreenColumnUsesExpandedText},
        {"ensureCursorVisible scrolls vertically and horizontally", ensuringVisibilityScrollsVerticallyAndHorizontally},
        {"horizontal motions clamp and treat zero count as one", horizontalMotionsClampAndTreatZeroCountAsOne},
        {"vertical motions preserve desired screen column", verticalMotionsPreserveTheDesiredScreenColumn},
        {"line-edge motions handle whitespace and counts", lineEdgeMotionsHandleWhitespaceEmptyLinesAndCounts},
        {"file motions implement gg and G count semantics", fileMotionsImplementGgAndGCountSemantics},
        {"H, M, and L use the visible region", windowRelativeMotionsUseTheVisibleRegion},
        {"page motions use viewport height and saturate", pageMotionsUseViewportHeightCountsAndSaturation},
        {"w distinguishes words, punctuation, and lines", wordForwardDistinguishesWordsPunctuationAndLines},
        {"b skips whitespace and crosses lines", wordBackwardSkipsWhitespaceAndCrossesLines},
        {"e finds class ends and crosses blank lines", wordEndFindsClassEndsAndCrossesBlankLines},
        {"horizontal motion replaces remembered vertical column", aHorizontalMoveReplacesTheRememberedVerticalColumn},
    });
}
