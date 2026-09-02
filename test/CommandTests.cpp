/*
Basic部分 模块测试
这是Command相关的测试,它只测试你是否正确实现了Command模块的公共接口约定.
Note: 如果你的vscode里面的c++ intellisense/clangd报错了,是正常的,因为它没有正确检测到文件的依赖关系,这对于我们的make test指令没有影响.
*/
#include "Command.hpp"

#include "TestSupport.hpp"

#include <utility>
#include <vector>

namespace {

sjtu::KeyEvent Character(char value) {
    return sjtu::KeyEvent::Character(static_cast<unsigned char>(value));
}

void CheckMotion(const sjtu::EditorAction& action, sjtu::Motion expected) {
    CHECK_EQ(action.kind_, sjtu::ActionKind::Move);
    CHECK(action.motion_.has_value());
    CHECK_EQ(*action.motion_, expected);
}

void CheckCommand(const sjtu::EditorAction& action, sjtu::ActionKind expected) {
    CHECK_EQ(action.kind_, expected);
    CHECK(!action.motion_.has_value());
}

void CharacterMotionsMapToBasicDirections() {
    const std::vector<std::pair<char, sjtu::Motion>> cases{
        {'h', sjtu::Motion::Left},
        {'j', sjtu::Motion::Down},
        {'k', sjtu::Motion::Up},
        {'l', sjtu::Motion::Right},
    };

    for (const auto& [key, motion] : cases) {
        sjtu::NormalModeParser parser;
        CheckMotion(parser.Feed(Character(key)), motion);
    }
}

void ArrowKeysMapToTheSameDirections() {
    const std::vector<std::pair<sjtu::KeyCode, sjtu::Motion>> cases{
        {sjtu::KeyCode::ArrowLeft, sjtu::Motion::Left},
        {sjtu::KeyCode::ArrowDown, sjtu::Motion::Down},
        {sjtu::KeyCode::ArrowUp, sjtu::Motion::Up},
        {sjtu::KeyCode::ArrowRight, sjtu::Motion::Right},
    };

    for (const auto& [key, motion] : cases) {
        sjtu::NormalModeParser parser;
        CheckMotion(parser.Feed({key, 0}), motion);
    }
}

void ModeSwitchKeysGenerateCommandsWithoutMotions() {
    const std::vector<std::pair<char, sjtu::ActionKind>> cases{
        {'i', sjtu::ActionKind::InsertBefore},
        {'a', sjtu::ActionKind::InsertAfter},
        {':', sjtu::ActionKind::EnterCommandLine},
    };

    for (const auto& [key, action] : cases) {
        sjtu::NormalModeParser parser;
        CheckCommand(parser.Feed(Character(key)), action);
    }
}

void UnsupportedKeysProduceAnEmptyAction() {
    sjtu::NormalModeParser parser;
    const std::vector<sjtu::KeyEvent> keys{
        Character('x'),
        Character('0'),
        {sjtu::KeyCode::Escape, 0},
        {sjtu::KeyCode::Enter, 0},
        {sjtu::KeyCode::Delete, 0},
        {sjtu::KeyCode::PageDown, 0},
    };

    for (const auto& key : keys) {
        const auto action = parser.Feed(key);
        CHECK_EQ(action.kind_, sjtu::ActionKind::None);
        CHECK(!action.motion_.has_value());
    }
}

void ParserDoesNotKeepStateBetweenBasicKeys() {
    sjtu::NormalModeParser parser;
    CHECK_EQ(parser.Feed(Character('g')).kind_, sjtu::ActionKind::None);
    CheckMotion(parser.Feed(Character('j')), sjtu::Motion::Down);
    CHECK_EQ(parser.Feed({sjtu::KeyCode::Escape, 0}).kind_,
             sjtu::ActionKind::None);
    CheckCommand(parser.Feed(Character('i')), sjtu::ActionKind::InsertBefore);
}

} // namespace

int main() {
    return test::Run({
        {"hjkl map to Basic motions", CharacterMotionsMapToBasicDirections},
        {"arrow keys map to Basic motions", ArrowKeysMapToTheSameDirections},
        {"i, a, and colon generate non-motion commands", ModeSwitchKeysGenerateCommandsWithoutMotions},
        {"unsupported keys generate an empty action", UnsupportedKeysProduceAnEmptyAction},
        {"Basic parser keeps no pending state", ParserDoesNotKeepStateBetweenBasicKeys},
    });
}
