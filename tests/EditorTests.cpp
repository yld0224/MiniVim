#include "Editor.hpp"
#include "Key.hpp"

#include "PtySupport.hpp"
#include "TestSupport.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace {

test::PtyResult runEditor(const std::filesystem::path& path,
                          const std::vector<test::InputChunk>& input) {
    return test::runInPty(
        [path](int ready) {
            sjtu::Editor editor(path);
            CHECK(editor.isRunning());
            test::signalReady(ready);
            editor.run();
            CHECK(!editor.isRunning());
        },
        input, 12, 80, 6000);
}

test::InputChunk escape() {
    return {std::string(1, '\x1b'), 160};
}

void unnamedBufferCanReportMissingNameThenSaveAsAndQuit() {
    test::TempDirectory temporary;
    const auto saved = temporary.file("created.txt");
    const auto result = runEditor(
        {},
        {{"iHello\tX\rWorld", 0}, escape(),
         {":w\r:w " + saved.string() + "\r:q\r", 0}});

    CHECK_PTY_SUCCESS(result);
    CHECK_CONTAINS(result.output, "no file name");
    CHECK_CONTAINS(result.output, "lines written");
    CHECK_EQ(test::readFile(saved), "Hello\tX\nWorld");
}

void quitRejectsDirtyBuffersAndForceQuitDiscardsThem() {
    test::TempDirectory temporary;
    const auto path = temporary.file("dirty.txt");
    test::writeFile(path, "old");

    const auto result = runEditor(
        path, {{"A!", 0}, escape(), {":q\r:q!\r", 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_CONTAINS(result.output,
                   "No write since last change (add ! to override)");
    CHECK_EQ(test::readFile(path), "old");
}

void globalControlQStopsTheEditorInInsertModeWithoutSaving() {
    test::TempDirectory temporary;
    const auto path = temporary.file("control-q.txt");
    test::writeFile(path, "old");
    std::string input = "A!";
    input.push_back(static_cast<char>(sjtu::controlKey('q')));

    const auto result = runEditor(path, {{input, 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_EQ(test::readFile(path), "old");
}

void insertBackspaceAndDeleteJoinAcrossLineBoundaries() {
    test::TempDirectory temporary;
    const auto backspacePath = temporary.file("backspace.txt");
    const auto deletePath = temporary.file("delete.txt");
    test::writeFile(backspacePath, "ab\ncd");
    test::writeFile(deletePath, "ab\ncd");

    const auto backspaceResult = runEditor(
        backspacePath,
        {{std::string("Gi") + static_cast<char>(0x7F), 0}, escape(),
         {":wq\r", 0}});
    CHECK_PTY_SUCCESS(backspaceResult);
    CHECK_EQ(test::readFile(backspacePath), "abcd");

    const auto deleteResult = runEditor(
        deletePath,
        {{"A\x1b[3~", 0}, escape(), {":wq\r", 0}});
    CHECK_PTY_SUCCESS(deleteResult);
    CHECK_EQ(test::readFile(deletePath), "abcd");
}

void insertionCommandsPlaceTextAndOpenLinesAtTheExpectedPositions() {
    test::TempDirectory temporary;
    const auto path = temporary.file("insert-actions.txt");
    test::writeFile(path, "  mid");

    const auto result = runEditor(
        path,
        {{"IA", 0}, escape(), {"AZ", 0}, escape(), {"0aX", 0}, escape(),
         {"olower", 0}, escape(), {"Oupper", 0}, escape(), {":wq\r", 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_EQ(test::readFile(path), " X AmidZ\nupper\nlower");
}

void normalDeletionAndJoinCommandsHonorCountsAndBoundaries() {
    test::TempDirectory temporary;
    const auto path = temporary.file("normal-edits.txt");
    test::writeFile(path, "abcde\n  two\nthree\nfour");

    const auto result = runEditor(path, {{"2xDJ2Jjdd:wq\r", 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_EQ(test::readFile(path), "two three");

    const auto countedPath = temporary.file("counted-dd.txt");
    test::writeFile(countedPath, "one\ntwo\nthree\nfour");
    const auto counted = runEditor(countedPath, {{"2dd:wq\r", 0}});
    CHECK_PTY_SUCCESS(counted);
    CHECK_EQ(test::readFile(countedPath), "three\nfour");
}

void insertEnterBackspaceAndDeleteHandleLineEdges() {
    test::TempDirectory temporary;
    const auto path = temporary.file("line-edges.txt");
    test::writeFile(path, "abcd");

    std::string firstChunk = "llia\rB";
    firstChunk.push_back(static_cast<char>(0x7F));
    const auto result = runEditor(
        path,
        {{firstChunk, 0}, escape(), {":wq\r", 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_EQ(test::readFile(path), "aba\ncd");

    const auto emptyPath = temporary.file("edge-noops.txt");
    test::writeFile(emptyPath, "");
    const auto noops = runEditor(
        emptyPath,
        {{std::string("i") + static_cast<char>(0x7F) + "\x1b[3~", 0},
         escape(), {":q!\r", 0}});
    CHECK_PTY_SUCCESS(noops);
    CHECK_EQ(test::readFile(emptyPath), "");
}

void commandLineEditingHelpErrorsAndLongNamesWorkTogether() {
    test::TempDirectory temporary;
    const auto source = temporary.file("source.txt");
    const auto copy = temporary.file("copy.txt");
    test::writeFile(source, "contents");

    std::string commands = ":garbage";
    commands.push_back(static_cast<char>(sjtu::controlKey('u')));
    commands += "  help  \r:bogux";
    commands.push_back(static_cast<char>(0x7F));
    commands += "s\r:write " + copy.string() + "\r:quit\r";

    const auto result = runEditor(source, {{commands, 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_CONTAINS(result.output,
                   "editor: hjkl wbe  i a I A o O  x dd D J  :w  :q");
    CHECK_CONTAINS(result.output, "Not an editor command: bogus");
    CHECK_CONTAINS(result.output, "line written");
    CHECK_EQ(test::readFile(copy), "contents");
}

void commandLineEscapeCancelsWithoutExecuting() {
    test::TempDirectory temporary;
    const auto source = temporary.file("source.txt");
    const auto unwanted = temporary.file("unwanted.txt");
    test::writeFile(source, "contents");

    const auto result = runEditor(
        source,
        {{":w " + unwanted.string(), 0}, escape(), {":q\r", 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK(!std::filesystem::exists(unwanted));
    CHECK_EQ(test::readFile(source), "contents");
}

void xSavesOnlyWhenNeededAndSupportsAnUnnamedTarget() {
    test::TempDirectory temporary;
    const auto named = temporary.file("named.txt");
    const auto unnamedTarget = temporary.file("from-unnamed.txt");
    test::writeFile(named, "old");

    const auto modified = runEditor(named, {{"A!", 0}, escape(), {":x\r", 0}});
    CHECK_PTY_SUCCESS(modified);
    CHECK_EQ(test::readFile(named), "old!");

    const auto unnamed = runEditor({}, {{":x " + unnamedTarget.string() + "\r", 0}});
    CHECK_PTY_SUCCESS(unnamed);
    CHECK(std::filesystem::exists(unnamedTarget));
    CHECK_EQ(test::readFile(unnamedTarget), "");

    const auto unchangedUnnamed = runEditor({}, {{":x\r", 0}});
    CHECK_PTY_SUCCESS(unchangedUnnamed);
}

void failedWriteAndQuitDoesNotQuitOrChangeTheOriginalFile() {
    test::TempDirectory temporary;
    const auto source = temporary.file("source.txt");
    test::writeFile(source, "old");

    const auto result = runEditor(
        source,
        {{"A!", 0}, escape(),
         {":wq " + temporary.path().string() + "\r:q!\r", 0}});
    CHECK_PTY_SUCCESS(result);
    CHECK_CONTAINS(result.output, "cannot write");
    CHECK_EQ(test::readFile(source), "old");
}

void missingInputFileFailsBeforeTerminalSetup() {
    test::TempDirectory temporary;
    const auto missing = temporary.file("missing.txt");
    const auto message = EXPECT_THROWS(std::runtime_error, sjtu::Editor(missing));
    CHECK_CONTAINS(message, "cannot open");
    CHECK_CONTAINS(message, missing.string());
}

} // namespace

int main() {
    return test::run({
        {"unnamed buffer reports missing name and supports save-as", unnamedBufferCanReportMissingNameThenSaveAsAndQuit},
        {"q rejects dirty buffers and q! discards changes", quitRejectsDirtyBuffersAndForceQuitDiscardsThem},
        {"Ctrl-Q stops insert mode without saving", globalControlQStopsTheEditorInInsertModeWithoutSaving},
        {"insert Backspace and Delete join lines", insertBackspaceAndDeleteJoinAcrossLineBoundaries},
        {"i/a/I/A/o/O place text and lines correctly", insertionCommandsPlaceTextAndOpenLinesAtTheExpectedPositions},
        {"x/dd/D/J honor counts and boundaries", normalDeletionAndJoinCommandsHonorCountsAndBoundaries},
        {"insert editing handles line edges", insertEnterBackspaceAndDeleteHandleLineEdges},
        {"command-line editing, help, errors, and write aliases work", commandLineEditingHelpErrorsAndLongNamesWorkTogether},
        {"command-line Escape cancels without executing", commandLineEscapeCancelsWithoutExecuting},
        {"x saves only when needed and supports unnamed targets", xSavesOnlyWhenNeededAndSupportsAnUnnamedTarget},
        {"failed wq does not quit or change the source file", failedWriteAndQuitDoesNotQuitOrChangeTheOriginalFile},
        {"missing input files fail before terminal setup", missingInputFileFailsBeforeTerminalSetup},
    });
}
