#include "TextLayout.hpp"

#include "TestSupport.hpp"

#include <string>

namespace {

void nextColumnHandlesPrintableTabsAndControls() {
    CHECK_EQ(sjtu::text::tabStop, 4U);
    CHECK_EQ(sjtu::text::nextScreenColumn(0, 'a'), 1U);
    CHECK_EQ(sjtu::text::nextScreenColumn(0, '\t'), 4U);
    CHECK_EQ(sjtu::text::nextScreenColumn(1, '\t'), 4U);
    CHECK_EQ(sjtu::text::nextScreenColumn(4, '\t'), 8U);
    CHECK_EQ(sjtu::text::nextScreenColumn(3, 0x00U), 5U);
    CHECK_EQ(sjtu::text::nextScreenColumn(3, 0x1FU), 5U);
    CHECK_EQ(sjtu::text::nextScreenColumn(3, 0x7FU), 5U);
    CHECK_EQ(sjtu::text::nextScreenColumn(3, 0x80U), 4U);
}

void screenColumnMapsPositionsAndClampsPastEnd() {
    const std::string line = "a\tb";
    CHECK_EQ(sjtu::text::screenColumn(line, 0), 0U);
    CHECK_EQ(sjtu::text::screenColumn(line, 1), 1U);
    CHECK_EQ(sjtu::text::screenColumn(line, 2), 4U);
    CHECK_EQ(sjtu::text::screenColumn(line, 3), 5U);
    CHECK_EQ(sjtu::text::screenColumn(line, 99), 5U);
    CHECK_EQ(sjtu::text::screenColumn("", 99), 0U);
}

void bufferColumnMapsCellsInsideWideGlyphs() {
    const std::string line = "a\tb";
    CHECK_EQ(sjtu::text::bufferColumn(line, 0), 0U);
    CHECK_EQ(sjtu::text::bufferColumn(line, 1), 1U);
    CHECK_EQ(sjtu::text::bufferColumn(line, 2), 1U);
    CHECK_EQ(sjtu::text::bufferColumn(line, 3), 1U);
    CHECK_EQ(sjtu::text::bufferColumn(line, 4), 2U);
    CHECK_EQ(sjtu::text::bufferColumn(line, 5), 2U);
    CHECK_EQ(sjtu::text::bufferColumn(line, 999), 2U);

    const std::string control(1, '\x01');
    CHECK_EQ(sjtu::text::bufferColumn(control + "x", 0), 0U);
    CHECK_EQ(sjtu::text::bufferColumn(control + "x", 1), 0U);
    CHECK_EQ(sjtu::text::bufferColumn(control + "x", 2), 1U);
    CHECK_EQ(sjtu::text::bufferColumn("", 20), 0U);
}

void expansionUsesCaretNotationAndTabStops() {
    std::string input;
    input.push_back('\0');
    input.push_back('\x01');
    input.push_back('\x1b');
    input.push_back('\x1f');
    input.push_back('\x7f');
    CHECK_EQ(sjtu::text::expandForDisplay(input), "^@^A^[^_^?");

    CHECK_EQ(sjtu::text::expandForDisplay("a\tb\t"), "a   b   ");
    CHECK_EQ(sjtu::text::expandForDisplay("\t\t"), "        ");
    CHECK_EQ(sjtu::text::expandForDisplay(""), "");
}

void expansionPreservesNonAsciiBytes() {
    std::string input;
    input.push_back(static_cast<char>(0x80U));
    input.push_back(static_cast<char>(0xFFU));
    CHECK_EQ(sjtu::text::expandForDisplay(input), input);
    CHECK_EQ(sjtu::text::screenColumn(input, input.size()), 2U);
}

} // namespace

int main() {
    return test::run({
        {"nextScreenColumn handles printable, tab, and control bytes", nextColumnHandlesPrintableTabsAndControls},
        {"screenColumn maps positions and clamps past line end", screenColumnMapsPositionsAndClampsPastEnd},
        {"bufferColumn maps cells inside expanded bytes", bufferColumnMapsCellsInsideWideGlyphs},
        {"expandForDisplay uses caret notation and tab stops", expansionUsesCaretNotationAndTabStops},
        {"expandForDisplay preserves non-ASCII bytes", expansionPreservesNonAsciiBytes},
    });
}
