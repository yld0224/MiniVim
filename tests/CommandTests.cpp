#include "Command.hpp"

#include "TestSupport.hpp"

#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

sjtu::KeyEvent character(char value) {
    return sjtu::KeyEvent::character(static_cast<unsigned char>(value));
}

void checkMotion(const sjtu::EditorAction& action, sjtu::Motion expected,
                 std::optional<std::size_t> count = std::nullopt) {
    CHECK_EQ(action.kind, sjtu::ActionKind::Move);
    CHECK(action.motion.has_value());
    CHECK_EQ(*action.motion, expected);
    CHECK_EQ(action.count, count);
}

void checkCommand(const sjtu::EditorAction& action, sjtu::ActionKind expected,
                  std::optional<std::size_t> count = std::nullopt) {
    CHECK_EQ(action.kind, expected);
    CHECK(!action.motion.has_value());
    CHECK_EQ(action.count, count);
}

void characterMotionsMapToTheExpectedMotion() {
    const std::vector<std::pair<char, sjtu::Motion>> cases{
        {'h', sjtu::Motion::Left},
        {'j', sjtu::Motion::Down},
        {'k', sjtu::Motion::Up},
        {'l', sjtu::Motion::Right},
        {'w', sjtu::Motion::WordForward},
        {'b', sjtu::Motion::WordBackward},
        {'e', sjtu::Motion::WordEnd},
        {'0', sjtu::Motion::LineStart},
        {'^', sjtu::Motion::FirstNonBlank},
        {'$', sjtu::Motion::LineEnd},
        {'G', sjtu::Motion::FileEnd},
        {'H', sjtu::Motion::WindowTop},
        {'M', sjtu::Motion::WindowMiddle},
        {'L', sjtu::Motion::WindowBottom},
    };

    for (const auto& [key, expected] : cases) {
        sjtu::NormalCommandParser parser;
        checkMotion(parser.feed(character(key)), expected);
        CHECK(parser.pendingDisplay().empty());
    }
}

void controlMotionsMapToPageOperations() {
    const std::vector<std::pair<char, sjtu::Motion>> cases{
        {'b', sjtu::Motion::PageUp},
        {'f', sjtu::Motion::PageDown},
        {'u', sjtu::Motion::HalfPageUp},
        {'d', sjtu::Motion::HalfPageDown},
    };

    for (const auto& [key, expected] : cases) {
        sjtu::NormalCommandParser parser;
        checkMotion(
            parser.feed(sjtu::KeyEvent::character(sjtu::controlKey(key))),
            expected);
    }
}

void specialKeysMapToMotions() {
    const std::vector<std::pair<sjtu::KeyCode, sjtu::Motion>> cases{
        {sjtu::KeyCode::ArrowLeft, sjtu::Motion::Left},
        {sjtu::KeyCode::ArrowRight, sjtu::Motion::Right},
        {sjtu::KeyCode::ArrowUp, sjtu::Motion::Up},
        {sjtu::KeyCode::ArrowDown, sjtu::Motion::Down},
        {sjtu::KeyCode::Home, sjtu::Motion::LineStart},
        {sjtu::KeyCode::End, sjtu::Motion::LineEnd},
        {sjtu::KeyCode::PageUp, sjtu::Motion::PageUp},
        {sjtu::KeyCode::PageDown, sjtu::Motion::PageDown},
    };

    for (const auto& [key, expected] : cases) {
        sjtu::NormalCommandParser parser;
        checkMotion(parser.feed({key, 0}), expected);
    }
}

void editingCharactersMapToActionsWithoutMotions() {
    const std::vector<std::pair<char, sjtu::ActionKind>> cases{
        {'i', sjtu::ActionKind::InsertBefore},
        {'a', sjtu::ActionKind::InsertAfter},
        {'I', sjtu::ActionKind::InsertAtFirstNonBlank},
        {'A', sjtu::ActionKind::InsertAtLineEnd},
        {'o', sjtu::ActionKind::OpenLineBelow},
        {'O', sjtu::ActionKind::OpenLineAbove},
        {'x', sjtu::ActionKind::DeleteCharacter},
        {'D', sjtu::ActionKind::DeleteToLineEnd},
        {'J', sjtu::ActionKind::JoinLines},
        {':', sjtu::ActionKind::EnterCommandLine},
    };

    for (const auto& [key, expected] : cases) {
        sjtu::NormalCommandParser parser;
        checkCommand(parser.feed(character(key)), expected);
    }
}

void twoKeyCommandsPreserveCountsAndClearPendingState() {
    sjtu::NormalCommandParser parser;
    CHECK_EQ(parser.feed(character('4')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.feed(character('g')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.pendingDisplay(), "4g");
    checkMotion(parser.feed(character('g')), sjtu::Motion::FileStart, 4U);
    CHECK(parser.pendingDisplay().empty());

    CHECK_EQ(parser.feed(character('2')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.feed(character('d')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.pendingDisplay(), "2d");
    checkCommand(parser.feed(character('d')), sjtu::ActionKind::DeleteLine, 2U);
    CHECK(parser.pendingDisplay().empty());
}

void decimalCountsApplyToBothMotionsAndCommands() {
    sjtu::NormalCommandParser parser;
    CHECK_EQ(parser.feed(character('1')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.feed(character('2')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.feed(character('0')).kind, sjtu::ActionKind::None);
    CHECK_EQ(parser.pendingDisplay(), "120");
    checkMotion(parser.feed(character('j')), sjtu::Motion::Down, 120U);

    parser.feed(character('3'));
    checkCommand(parser.feed(character('x')), sjtu::ActionKind::DeleteCharacter, 3U);

    checkMotion(parser.feed(character('0')), sjtu::Motion::LineStart);
}

void countOverflowSaturatesAtSizeMaximum() {
    sjtu::NormalCommandParser parser;
    for (int digit = 0; digit < 200; ++digit) {
        parser.feed(character('9'));
    }
    CHECK_EQ(parser.pendingDisplay(),
             std::to_string(std::numeric_limits<std::size_t>::max()));
    const auto action = parser.feed(character('l'));
    checkMotion(action, sjtu::Motion::Right,
                std::numeric_limits<std::size_t>::max());
}

void invalidInputAndEscapeResetAllPendingState() {
    sjtu::NormalCommandParser parser;
    parser.feed(character('8'));
    parser.feed(character('g'));
    CHECK_EQ(parser.pendingDisplay(), "8g");
    const auto invalidPrefix = parser.feed(character('x'));
    CHECK_EQ(invalidPrefix.kind, sjtu::ActionKind::None);
    CHECK(!invalidPrefix.motion.has_value());
    CHECK(!invalidPrefix.count.has_value());
    CHECK(parser.pendingDisplay().empty());

    parser.feed(character('7'));
    const auto escape = parser.feed({sjtu::KeyCode::Escape, 0});
    CHECK_EQ(escape.kind, sjtu::ActionKind::None);
    CHECK(parser.pendingDisplay().empty());

    parser.feed(character('6'));
    const auto enter = parser.feed({sjtu::KeyCode::Enter, 0});
    CHECK_EQ(enter.kind, sjtu::ActionKind::None);
    CHECK(parser.pendingDisplay().empty());

    parser.feed(character('5'));
    const auto unknown = parser.feed(character('?'));
    CHECK_EQ(unknown.kind, sjtu::ActionKind::None);
    CHECK(parser.pendingDisplay().empty());
}

void explicitResetDropsCountAndPrefix() {
    sjtu::NormalCommandParser parser;
    parser.feed(character('9'));
    parser.feed(character('d'));
    CHECK_EQ(parser.pendingDisplay(), "9d");
    parser.reset();
    CHECK(parser.pendingDisplay().empty());
    checkMotion(parser.feed(character('j')), sjtu::Motion::Down);
}

void controlQAlwaysProducesQuitAndResetsState() {
    sjtu::NormalCommandParser parser;
    parser.feed(character('4'));
    parser.feed(character('g'));
    const auto quit =
        parser.feed(sjtu::KeyEvent::character(sjtu::controlKey('q')));
    checkCommand(quit, sjtu::ActionKind::Quit);
    CHECK(parser.pendingDisplay().empty());
}

void noneAndCommandActionsNeverCarryAMotion() {
    sjtu::NormalCommandParser parser;
    const auto none = parser.feed(character('1'));
    CHECK_EQ(none.kind, sjtu::ActionKind::None);
    CHECK(!none.motion.has_value());

    const auto command = parser.feed(character('i'));
    CHECK_EQ(command.kind, sjtu::ActionKind::InsertBefore);
    CHECK(!command.motion.has_value());

    const auto motion = parser.feed(character('h'));
    CHECK_EQ(motion.kind, sjtu::ActionKind::Move);
    CHECK(motion.motion.has_value());
}

} // namespace

int main() {
    return test::run({
        {"character motions map to Motion values", characterMotionsMapToTheExpectedMotion},
        {"control keys map to page motions", controlMotionsMapToPageOperations},
        {"special keys map to motions", specialKeysMapToMotions},
        {"editing keys map to actions without motions", editingCharactersMapToActionsWithoutMotions},
        {"gg and dd preserve counts and clear pending state", twoKeyCommandsPreserveCountsAndClearPendingState},
        {"decimal counts apply to motions and commands", decimalCountsApplyToBothMotionsAndCommands},
        {"count overflow saturates at size_t maximum", countOverflowSaturatesAtSizeMaximum},
        {"invalid input and Escape reset pending state", invalidInputAndEscapeResetAllPendingState},
        {"reset drops count and prefix", explicitResetDropsCountAndPrefix},
        {"Ctrl-Q produces Quit and resets state", controlQAlwaysProducesQuitAndResetsState},
        {"only Move actions carry a motion", noneAndCommandActionsNeverCarryAMotion},
    });
}
