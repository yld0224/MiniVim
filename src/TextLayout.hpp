/*
TextLayout.hpp
这些辅助函数负责字符下标和显示列之间的换算,你可以在任意文件使用它们
这里的显示列是整行展开后的列号,还没有减去窗口的横向滚动偏移.
*/
#ifndef MINIVIM_TEXT_LAYOUT_HPP
#define MINIVIM_TEXT_LAYOUT_HPP

#include <string_view>
#include <string>

namespace sjtu::text {

inline constexpr size_t tabStop = 4; //Tab对齐到下一个4的倍数列,不一定总占4列

inline size_t NextScreenColumn(std::size_t column, unsigned char value) {
    //返回从column开始显示value后的位置:Tab跳到下一个制表位,普通字符占1列
    if (value == '\t') { return column + (tabStop - (column % tabStop)); }
    return column + 1;
}

inline std::size_t LastColumn(const std::string& line) {
    //返回Normal模式允许到达的最后一个字符下标
    return line.empty() ? 0 : line.size() - 1;
}


inline std::size_t RenderColumnToBufferColumn(std::string_view line, std::size_t render_column) {
    //从当前Render出来的视图的光标所在列转换到Buffer中的实际列
    if (line.empty()) {
        return 0;
    }

    size_t current = 0;
    for (size_t index = 0; index < line.size(); ++index) {
        auto next = NextScreenColumn(current, static_cast<unsigned char>(line[index]));
        if (render_column < next) {
            return index;
        }
        current = next;
    }
    return line.size() - 1;
}

inline size_t BufferColumnToRenderColumn(std::string_view line, size_t buffer_column) {
    //从Buffer中实际Cursor位置所在列转换到Render时Cursor所在列
    size_t column = 0;
    for (size_t index = 0; index < buffer_column; ++index) {
        column = NextScreenColumn(column, static_cast<unsigned char>(line[index]));
    }
    return column;
}
} 

#endif // MINIVIM_TEXT_LAYOUT_HPP
