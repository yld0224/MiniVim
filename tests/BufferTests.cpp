#include "Buffer.hpp"

#include "TestSupport.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

sjtu::Buffer lines(std::initializer_list<std::string> values) {
    return sjtu::Buffer(std::vector<std::string>(values));
}

template <typename Function>
void checkOutOfRangeInChild(Function&& function, const char* operation) {
    const auto child = ::fork();
    CHECK_NE(child, -1);
    if (child == 0) {
        try {
            function();
        } catch (const std::out_of_range&) {
            ::_exit(0);
        } catch (...) {
            ::_exit(2);
        }
        ::_exit(3);
    }

    int status = 0;
    CHECK_EQ(::waitpid(child, &status, 0), child);
    if (WIFSIGNALED(status)) {
        test::fail(operation, __FILE__, __LINE__,
                   "terminated by signal " + std::to_string(WTERMSIG(status)));
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        test::fail(operation, __FILE__, __LINE__,
                   "did not throw std::out_of_range");
    }
}

void constructorsMaintainTheNonEmptyInvariant() {
    const sjtu::Buffer unnamed;
    CHECK_EQ(unnamed.lineCount(), 1U);
    CHECK(unnamed.line(0).empty());
    CHECK(unnamed.path().empty());
    CHECK_EQ(unnamed.displayName(), "[No Name]");
    CHECK(!unnamed.isModified());

    const sjtu::Buffer emptyVector(std::vector<std::string>{});
    CHECK_EQ(emptyVector.lineCount(), 1U);
    CHECK(emptyVector.line(0).empty());

    const auto namedPath = std::filesystem::path("directory/sample.txt");
    const sjtu::Buffer named(std::vector<std::string>{"one", "two"}, namedPath);
    CHECK_EQ(named.lineCount(), 2U);
    CHECK_EQ(named.line(0), "one");
    CHECK_EQ(named.line(1), "two");
    CHECK_EQ(named.path(), namedPath);
    CHECK_EQ(named.displayName(), namedPath.string());
    CHECK(!named.isModified());
}

void fileConstructorHandlesLineEndingsAndEmptyFiles() {
    test::TempDirectory temporary;
    const auto noNewline = temporary.file("no-newline.txt");
    const auto newline = temporary.file("newline.txt");
    const auto crlf = temporary.file("crlf.txt");
    const auto empty = temporary.file("empty.txt");
    test::writeFile(noNewline, "alpha\nbeta");
    test::writeFile(newline, "alpha\nbeta\n");
    test::writeFile(crlf, "alpha\r\nbeta\r\n");
    test::writeFile(empty, "");

    const sjtu::Buffer first(noNewline);
    CHECK_EQ(first.lineCount(), 2U);
    CHECK_EQ(first.line(0), "alpha");
    CHECK_EQ(first.line(1), "beta");
    CHECK_EQ(first.path(), noNewline);

    sjtu::Buffer second(newline);
    second.save();
    CHECK_EQ(test::readFile(newline), "alpha\nbeta\n");

    const sjtu::Buffer windows(crlf);
    CHECK_EQ(windows.lineCount(), 2U);
    CHECK_EQ(windows.line(0), "alpha");
    CHECK_EQ(windows.line(1), "beta");

    const sjtu::Buffer blank(empty);
    CHECK_EQ(blank.lineCount(), 1U);
    CHECK(blank.line(0).empty());

    const auto missing = temporary.file("missing.txt");
    const auto message = EXPECT_THROWS(std::runtime_error, sjtu::Buffer(missing));
    CHECK_CONTAINS(message, "cannot open");
    CHECK_CONTAINS(message, missing.string());
}

void lineAccessChecksBounds() {
    const auto buffer = lines({"only"});
    CHECK_EQ(buffer.line(0), "only");
    EXPECT_THROWS(std::out_of_range, buffer.line(1));
}

void insertCharacterSupportsAllValidInsertionPoints() {
    auto buffer = lines({"bc", ""});
    buffer.insertCharacter(0, 0, 'a');
    buffer.insertCharacter(0, 3, 'd');
    buffer.insertCharacter(1, 0, 'x');
    CHECK_EQ(buffer.line(0), "abcd");
    CHECK_EQ(buffer.line(1), "x");
    CHECK(buffer.isModified());

    EXPECT_THROWS(std::out_of_range, buffer.insertCharacter(0, 5, '!'));
    EXPECT_THROWS(std::out_of_range, buffer.insertCharacter(5, 0, '!'));
}

void eraseCharacterChecksRowsColumnsAndLastCharacter() {
    auto buffer = lines({"abc", "x"});
    buffer.eraseCharacter(0, 1);
    buffer.eraseCharacter(1, 0);
    CHECK_EQ(buffer.line(0), "ac");
    CHECK(buffer.line(1).empty());
    CHECK(buffer.isModified());

    EXPECT_THROWS(std::out_of_range, buffer.eraseCharacter(0, 2));
    EXPECT_THROWS(std::out_of_range, buffer.eraseCharacter(9, 0));
}

void splitLineHandlesStartMiddleAndEnd() {
    auto middle = lines({"abcd"});
    middle.splitLine(0, 2);
    CHECK_EQ(middle.lineCount(), 2U);
    CHECK_EQ(middle.line(0), "ab");
    CHECK_EQ(middle.line(1), "cd");

    auto edges = lines({"abc"});
    edges.splitLine(0, 0);
    CHECK_EQ(edges.line(0), "");
    CHECK_EQ(edges.line(1), "abc");
    edges.splitLine(1, 3);
    CHECK_EQ(edges.line(1), "abc");
    CHECK_EQ(edges.line(2), "");

    EXPECT_THROWS(std::out_of_range, edges.splitLine(1, 4));
    EXPECT_THROWS(std::out_of_range, edges.splitLine(9, 0));
}

void insertLineSupportsBeginningMiddleAndEnd() {
    auto buffer = lines({"b", "d"});
    buffer.insertLine(0, "a");
    buffer.insertLine(2, "c");
    buffer.insertLine(buffer.lineCount(), "e");
    CHECK_EQ(buffer.lineCount(), 5U);
    CHECK_EQ(buffer.line(0), "a");
    CHECK_EQ(buffer.line(1), "b");
    CHECK_EQ(buffer.line(2), "c");
    CHECK_EQ(buffer.line(3), "d");
    CHECK_EQ(buffer.line(4), "e");
    CHECK(buffer.isModified());
}

void insertLineRejectsRowsPastTheInsertionBoundary() {
    auto insert = lines({"only"});
    checkOutOfRangeInChild([&] { insert.insertLine(2, "too far"); },
                           "insertLine rejects row > lineCount");
}

void eraseLinesRejectsRowsPastTheBuffer() {
    auto erase = lines({"only"});
    checkOutOfRangeInChild([&] { erase.eraseLines(2, 1); },
                           "eraseLines rejects row >= lineCount");
}

void separatedJoinRejectsTheLastRow() {
    auto separatedJoin = lines({"only"});
    checkOutOfRangeInChild(
        [&] { separatedJoin.joinWithNextLineSeparated(0); },
        "joinWithNextLineSeparated rejects the last row");
}

void eraseLinesClampsCountAndKeepsOneLine() {
    auto buffer = lines({"zero", "one", "two", "three"});
    buffer.eraseLines(1, 2);
    CHECK_EQ(buffer.lineCount(), 2U);
    CHECK_EQ(buffer.line(0), "zero");
    CHECK_EQ(buffer.line(1), "three");
    buffer.eraseLines(1, 99);
    CHECK_EQ(buffer.lineCount(), 1U);
    CHECK_EQ(buffer.line(0), "zero");

    auto all = lines({"a", "b"});
    all.eraseLines(0, 99);
    CHECK_EQ(all.lineCount(), 1U);
    CHECK(all.line(0).empty());
    CHECK(all.isModified());

    auto alreadyEmpty = lines({""});
    alreadyEmpty.eraseLines(0);
    CHECK_EQ(alreadyEmpty.lineCount(), 1U);
    CHECK(!alreadyEmpty.isModified());
}

void erasingZeroLinesIsANoOp() {
    auto buffer = lines({"one", "two"});
    buffer.eraseLines(1, 0);
    CHECK_EQ(buffer.lineCount(), 2U);
    CHECK_EQ(buffer.line(0), "one");
    CHECK_EQ(buffer.line(1), "two");
    CHECK(!buffer.isModified());
}

void eraseToLineEndHandlesMiddleStartAndEnd() {
    auto middle = lines({"abcdef"});
    middle.eraseToLineEnd(0, 3);
    CHECK_EQ(middle.line(0), "abc");
    CHECK(middle.isModified());

    auto start = lines({"abcdef"});
    start.eraseToLineEnd(0, 0);
    CHECK(start.line(0).empty());

    auto end = lines({"abcdef"});
    end.eraseToLineEnd(0, 6);
    CHECK_EQ(end.line(0), "abcdef");
    CHECK(!end.isModified());

    EXPECT_THROWS(std::out_of_range, end.eraseToLineEnd(0, 7));
    EXPECT_THROWS(std::out_of_range, end.eraseToLineEnd(2, 0));
}

void joinsImplementLiteralAndVimStyleSemantics() {
    auto literal = lines({"left", "right", "tail"});
    literal.joinWithNextLine(0);
    CHECK_EQ(literal.lineCount(), 2U);
    CHECK_EQ(literal.line(0), "leftright");
    CHECK_EQ(literal.line(1), "tail");
    CHECK(literal.isModified());
    EXPECT_THROWS(std::out_of_range, literal.joinWithNextLine(1));

    auto separated = lines({"left", "  \tright"});
    separated.joinWithNextLineSeparated(0);
    CHECK_EQ(separated.line(0), "left right");

    auto existingSpace = lines({"left ", "  right"});
    existingSpace.joinWithNextLineSeparated(0);
    CHECK_EQ(existingSpace.line(0), "left right");

    auto blank = lines({"left", " \t "});
    blank.joinWithNextLineSeparated(0);
    CHECK_EQ(blank.line(0), "left");

    auto emptyFirst = lines({"", " next"});
    emptyFirst.joinWithNextLineSeparated(0);
    CHECK_EQ(emptyFirst.line(0), "next");
}

void saveAndSaveAsManageNameDirtyStateAndBytes() {
    test::TempDirectory temporary;
    const auto firstPath = temporary.file("first.txt");
    const auto secondPath = temporary.file("second.txt");

    auto buffer = lines({"alpha", "beta"});
    buffer.insertCharacter(0, 5, '!');
    CHECK(buffer.isModified());
    const auto unnamedMessage = EXPECT_THROWS(std::runtime_error, buffer.save());
    CHECK_EQ(unnamedMessage, "no file name");
    CHECK(buffer.isModified());
    CHECK(buffer.path().empty());

    buffer.saveAs(firstPath);
    CHECK_EQ(buffer.path(), firstPath);
    CHECK_EQ(buffer.displayName(), firstPath.string());
    CHECK(!buffer.isModified());
    CHECK_EQ(test::readFile(firstPath), "alpha!\nbeta");

    buffer.eraseCharacter(0, 5);
    buffer.save();
    CHECK(!buffer.isModified());
    CHECK_EQ(test::readFile(firstPath), "alpha\nbeta");

    buffer.insertLine(2, "gamma");
    buffer.saveAs(secondPath);
    CHECK_EQ(buffer.path(), secondPath);
    CHECK_EQ(test::readFile(secondPath), "alpha\nbeta\ngamma");
    CHECK_EQ(test::readFile(firstPath), "alpha\nbeta");
}

void savePreservesOriginalTrailingNewlinePolicy() {
    test::TempDirectory temporary;
    const auto withNewline = temporary.file("with-newline.txt");
    const auto withoutNewline = temporary.file("without-newline.txt");
    test::writeFile(withNewline, "a\nb\n");
    test::writeFile(withoutNewline, "a\nb");

    sjtu::Buffer first(withNewline);
    first.insertCharacter(1, 1, '!');
    first.save();
    CHECK_EQ(test::readFile(withNewline), "a\nb!\n");

    sjtu::Buffer second(withoutNewline);
    second.insertCharacter(1, 1, '!');
    second.save();
    CHECK_EQ(test::readFile(withoutNewline), "a\nb!");
}

void saveFailuresDoNotRenameOrCleanTheBuffer() {
    test::TempDirectory temporary;
    auto buffer = lines({"data"});
    buffer.insertCharacter(0, 4, '!');

    const auto emptyMessage = EXPECT_THROWS(
        std::runtime_error, buffer.saveAs(std::filesystem::path{}));
    CHECK_EQ(emptyMessage, "no file name");
    CHECK(buffer.path().empty());
    CHECK(buffer.isModified());

    const auto message = EXPECT_THROWS(std::runtime_error, buffer.saveAs(temporary.path()));
    CHECK_CONTAINS(message, "cannot write");
    CHECK(buffer.path().empty());
    CHECK(buffer.isModified());
}

} // namespace

int main() {
    return test::run({
        {"constructors maintain a non-empty buffer", constructorsMaintainTheNonEmptyInvariant},
        {"file constructor handles newline variants and missing files", fileConstructorHandlesLineEndingsAndEmptyFiles},
        {"line access checks bounds", lineAccessChecksBounds},
        {"insertCharacter supports all valid insertion points", insertCharacterSupportsAllValidInsertionPoints},
        {"eraseCharacter checks rows and columns", eraseCharacterChecksRowsColumnsAndLastCharacter},
        {"splitLine handles start, middle, and end", splitLineHandlesStartMiddleAndEnd},
        {"insertLine supports beginning, middle, and end", insertLineSupportsBeginningMiddleAndEnd},
        {"insertLine rejects rows past lineCount", insertLineRejectsRowsPastTheInsertionBoundary},
        {"eraseLines rejects rows past the buffer", eraseLinesRejectsRowsPastTheBuffer},
        {"separated join rejects the last row", separatedJoinRejectsTheLastRow},
        {"eraseLines clamps count and keeps one line", eraseLinesClampsCountAndKeepsOneLine},
        {"eraseLines with zero count is a no-op", erasingZeroLinesIsANoOp},
        {"eraseToLineEnd handles start, middle, and end", eraseToLineEndHandlesMiddleStartAndEnd},
        {"join operations implement literal and Vim-style semantics", joinsImplementLiteralAndVimStyleSemantics},
        {"save and saveAs manage names, dirty state, and bytes", saveAndSaveAsManageNameDirtyStateAndBytes},
        {"save preserves the source trailing-newline policy", savePreservesOriginalTrailingNewlinePolicy},
        {"save failures do not rename or clean the buffer", saveFailuresDoNotRenameOrCleanTheBuffer},
    });
}
