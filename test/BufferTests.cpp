/*
Basic部分 模块测试
这是Buffer相关的测试,它只测试你是否正确实现了Buffer模块的公共接口约定.
Note: 如果你的vscode里面的c++ intellisense/clangd报错了,是正常的,因为它没有正确检测到文件的依赖关系,这对于我们的make test指令没有影响.
*/
#include "Buffer.hpp"

#include "TestSupport.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

sjtu::Buffer Lines(std::initializer_list<std::string> values,
                   const std::filesystem::path& path = {}) {
    return sjtu::Buffer(std::vector<std::string>(values), path);
}

void ConstructorsKeepAtLeastOneLine() {
    const sjtu::Buffer unnamed;
    CHECK_EQ(unnamed.GetLineCount(), 1U);
    CHECK(unnamed.GetLineAt(0).empty());
    CHECK_EQ(unnamed.GetDisplayName(), "[No Name]");
    CHECK(!unnamed.IsModified());

    const sjtu::Buffer empty_vector(std::vector<std::string>{});
    CHECK_EQ(empty_vector.GetLineCount(), 1U);
    CHECK(empty_vector.GetLineAt(0).empty());

    const sjtu::Buffer named(std::vector<std::string>{"one", "two"},
                             "sample.txt");
    CHECK_EQ(named.GetLineCount(), 2U);
    CHECK_EQ(named.GetLineAt(0), "one");
    CHECK_EQ(named.GetLineAt(1), "two");
    CHECK_EQ(named.GetDisplayName(), "sample.txt");
    CHECK(!named.IsModified());
}

void FileConstructorHandlesEmptyLfAndCrlfFiles() {
    test::TempDirectory temporary;
    const auto empty = temporary.File("empty.txt");
    const auto no_newline = temporary.File("no-newline.txt");
    const auto newline = temporary.File("newline.txt");
    const auto crlf = temporary.File("crlf.txt");
    test::WriteFile(empty, "");
    test::WriteFile(no_newline, "alpha\nbeta");
    test::WriteFile(newline, "alpha\nbeta\n");
    test::WriteFile(crlf, "alpha\r\nbeta\r\n");

    const sjtu::Buffer blank(empty);
    CHECK_EQ(blank.GetLineCount(), 1U);
    CHECK(blank.GetLineAt(0).empty());

    sjtu::Buffer without_final_newline(no_newline);
    CHECK_EQ(without_final_newline.GetLineCount(), 2U);
    CHECK_EQ(without_final_newline.GetLineAt(0), "alpha");
    CHECK_EQ(without_final_newline.GetLineAt(1), "beta");
    without_final_newline.Save();
    CHECK_EQ(test::ReadFile(no_newline), "alpha\nbeta");

    sjtu::Buffer with_final_newline(newline);
    with_final_newline.Save();
    CHECK_EQ(test::ReadFile(newline), "alpha\nbeta\n");

    const sjtu::Buffer windows(crlf);
    CHECK_EQ(windows.GetLineCount(), 2U);
    CHECK_EQ(windows.GetLineAt(0), "alpha");
    CHECK_EQ(windows.GetLineAt(1), "beta");
}

void MissingFilesAndInvalidRowsThrow() {
    test::TempDirectory temporary;
    const auto missing = temporary.File("missing.txt");
    const auto message = EXPECT_THROWS(std::runtime_error, sjtu::Buffer(missing));
    CHECK_CONTAINS(message, "cannot open");
    CHECK_CONTAINS(message, missing.string());

    const auto buffer = Lines({"only"});
    EXPECT_THROWS(std::out_of_range, buffer.GetLineAt(1));
}

void InsertCharacterSupportsEveryValidPosition() {
    auto buffer = Lines({"bc", ""});
    buffer.InsertCharacter(0, 0, 'a');
    buffer.InsertCharacter(0, 3, 'd');
    buffer.InsertCharacter(1, 0, 'x');
    CHECK_EQ(buffer.GetLineAt(0), "abcd");
    CHECK_EQ(buffer.GetLineAt(1), "x");
    CHECK(buffer.IsModified());

    EXPECT_THROWS(std::out_of_range, buffer.InsertCharacter(0, 5, '!'));
    EXPECT_THROWS(std::out_of_range, buffer.InsertCharacter(5, 0, '!'));
}

void EraseCharacterChecksCharacterBounds() {
    auto buffer = Lines({"abc", "x"});
    buffer.EraseCharacter(0, 1);
    buffer.EraseCharacter(1, 0);
    CHECK_EQ(buffer.GetLineAt(0), "ac");
    CHECK(buffer.GetLineAt(1).empty());
    CHECK(buffer.IsModified());

    EXPECT_THROWS(std::out_of_range, buffer.EraseCharacter(0, 2));
    EXPECT_THROWS(std::out_of_range, buffer.EraseCharacter(9, 0));
}

void SplitLineHandlesStartMiddleAndEnd() {
    auto middle = Lines({"abcd"});
    middle.SplitLine(0, 2);
    CHECK_EQ(middle.GetLineCount(), 2U);
    CHECK_EQ(middle.GetLineAt(0), "ab");
    CHECK_EQ(middle.GetLineAt(1), "cd");

    auto edges = Lines({"abc"});
    edges.SplitLine(0, 0);
    CHECK_EQ(edges.GetLineAt(0), "");
    CHECK_EQ(edges.GetLineAt(1), "abc");
    edges.SplitLine(1, 3);
    CHECK_EQ(edges.GetLineAt(1), "abc");
    CHECK_EQ(edges.GetLineAt(2), "");
    CHECK(edges.IsModified());

    EXPECT_THROWS(std::out_of_range, edges.SplitLine(1, 4));
    EXPECT_THROWS(std::out_of_range, edges.SplitLine(9, 0));
}

void JoinLineConcatenatesWithTheNextLine() {
    auto buffer = Lines({"left", "right", "tail"});
    buffer.JoinLine(0);
    CHECK_EQ(buffer.GetLineCount(), 2U);
    CHECK_EQ(buffer.GetLineAt(0), "leftright");
    CHECK_EQ(buffer.GetLineAt(1), "tail");
    CHECK(buffer.IsModified());
    EXPECT_THROWS(std::out_of_range, buffer.JoinLine(1));

    auto empty = Lines({"", "next"});
    empty.JoinLine(0);
    CHECK_EQ(empty.GetLineCount(), 1U);
    CHECK_EQ(empty.GetLineAt(0), "next");
}

void SaveAndSaveAsManageNameBytesAndDirtyState() {
    test::TempDirectory temporary;
    const auto first = temporary.File("first.txt");
    const auto second = temporary.File("second.txt");

    auto buffer = Lines({"alpha", "beta"});
    buffer.InsertCharacter(0, 5, '!');
    CHECK(buffer.IsModified());
    CHECK_EQ(EXPECT_THROWS(std::runtime_error, buffer.Save()), "no file name");
    CHECK(buffer.IsModified());

    buffer.SaveAs(first);
    CHECK_EQ(buffer.GetDisplayName(), first.string());
    CHECK(!buffer.IsModified());
    CHECK_EQ(test::ReadFile(first), "alpha!\nbeta");

    buffer.EraseCharacter(0, 5);
    buffer.Save();
    CHECK(!buffer.IsModified());
    CHECK_EQ(test::ReadFile(first), "alpha\nbeta");

    buffer.InsertCharacter(1, 4, '!');
    buffer.SaveAs(second);
    CHECK_EQ(buffer.GetDisplayName(), second.string());
    CHECK_EQ(test::ReadFile(second), "alpha\nbeta!");
    CHECK_EQ(test::ReadFile(first), "alpha\nbeta");
}

void FailedSaveAsKeepsTheOldNameAndDirtyState() {
    test::TempDirectory temporary;
    auto buffer = Lines({"data"});
    buffer.InsertCharacter(0, 4, '!');

    CHECK_EQ(EXPECT_THROWS(std::runtime_error,
                           buffer.SaveAs(std::filesystem::path{})),
             "no file name");
    CHECK_EQ(buffer.GetDisplayName(), "[No Name]");
    CHECK(buffer.IsModified());

    const auto message =
        EXPECT_THROWS(std::runtime_error, buffer.SaveAs(temporary.Path()));
    CHECK_CONTAINS(message, "cannot write");
    CHECK_EQ(buffer.GetDisplayName(), "[No Name]");
    CHECK(buffer.IsModified());
}

} // namespace

int main() {
    return test::Run({
        {"constructors keep a non-empty Buffer", ConstructorsKeepAtLeastOneLine},
        {"file constructor handles empty, LF, and CRLF files", FileConstructorHandlesEmptyLfAndCrlfFiles},
        {"missing files and invalid rows throw", MissingFilesAndInvalidRowsThrow},
        {"InsertCharacter supports every valid position", InsertCharacterSupportsEveryValidPosition},
        {"EraseCharacter checks character bounds", EraseCharacterChecksCharacterBounds},
        {"SplitLine handles start, middle, and end", SplitLineHandlesStartMiddleAndEnd},
        {"JoinLine concatenates with the next line", JoinLineConcatenatesWithTheNextLine},
        {"Save and SaveAs manage name, bytes, and dirty state", SaveAndSaveAsManageNameBytesAndDirtyState},
        {"failed SaveAs preserves name and dirty state", FailedSaveAsKeepsTheOldNameAndDirtyState},
    });
}
