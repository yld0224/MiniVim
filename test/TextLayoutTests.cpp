#include "TextLayout.hpp"

#include "TestSupport.hpp"

#include <string>

namespace {

void NextColumnHandlesPrintableTabAndControlBytes() {
    CHECK_EQ(sjtu::text::tabStop, 4U);
    CHECK_EQ(sjtu::text::NextScreenColumn(0, 'a'), 1U);
    CHECK_EQ(sjtu::text::NextScreenColumn(0, '\t'), 4U);
    CHECK_EQ(sjtu::text::NextScreenColumn(1, '\t'), 4U);
    CHECK_EQ(sjtu::text::NextScreenColumn(4, '\t'), 8U);
    CHECK_EQ(sjtu::text::NextScreenColumn(3, 0x00U), 5U);
    CHECK_EQ(sjtu::text::NextScreenColumn(3, 0x1FU), 5U);
    CHECK_EQ(sjtu::text::NextScreenColumn(3, 0x7FU), 5U);
    CHECK_EQ(sjtu::text::NextScreenColumn(3, 0x80U), 4U);
}

void LastColumnHandlesEmptyAndNonEmptyLines() {
    CHECK_EQ(sjtu::text::LastColumn(""), 0U);
    CHECK_EQ(sjtu::text::LastColumn("x"), 0U);
    CHECK_EQ(sjtu::text::LastColumn("abcd"), 3U);
}

void RenderColumnMapsToNormalModeBufferPositions() {
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn("", 100), 0U);

    const std::string plain = "abcd";
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(plain, 0), 0U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(plain, 2), 2U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(plain, 4), 3U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(plain, 100), 3U);

    const std::string tabbed = "a\tb";
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(tabbed, 0), 0U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(tabbed, 1), 1U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(tabbed, 2), 1U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(tabbed, 3), 1U);
    CHECK_EQ(sjtu::text::RenderColumnToBufferColumn(tabbed, 4), 2U);
}

void BufferColumnMapsCharactersAndLineEndToRenderColumns() {
    const std::string line = "a\tb";
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn(line, 0), 0U);
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn(line, 1), 1U);
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn(line, 2), 4U);
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn(line, 3), 5U);

    const std::string control = std::string(1, '\x01') + "x";
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn(control, 1), 2U);
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn(control, 2), 3U);
    CHECK_EQ(sjtu::text::BufferColumnToRenderColumn("", 0), 0U);
}

} // namespace

int main() {
    return test::Run({
        {"NextScreenColumn handles printable, tab, and control bytes", NextColumnHandlesPrintableTabAndControlBytes},
        {"LastColumn handles empty and non-empty lines", LastColumnHandlesEmptyAndNonEmptyLines},
        {"render columns map to Normal-mode buffer positions", RenderColumnMapsToNormalModeBufferPositions},
        {"buffer columns and line end map to render columns", BufferColumnMapsCharactersAndLineEndToRenderColumns},
    });
}
