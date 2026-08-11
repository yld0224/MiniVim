#include "Key.hpp"

#include "TestSupport.hpp"

#include <type_traits>

namespace {

void controlKeyUsesAsciiControlMask() {
    static_assert(noexcept(sjtu::controlKey('q')));
    static_assert(sjtu::controlKey('@') == 0U);
    static_assert(sjtu::controlKey('A') == 1U);
    static_assert(sjtu::controlKey('q') == 17U);
    static_assert(sjtu::controlKey('[') == 27U);
    static_assert(sjtu::controlKey('?') == 31U);

    CHECK_EQ(sjtu::controlKey('a'), sjtu::controlKey('A'));
}

void characterFactoryPreservesEveryByte() {
    constexpr auto nul = sjtu::KeyEvent::character(0U);
    constexpr auto ascii = sjtu::KeyEvent::character('x');
    constexpr auto high = sjtu::KeyEvent::character(0xFFU);

    static_assert(nul.code == sjtu::KeyCode::Character && nul.value == 0U);
    static_assert(ascii.code == sjtu::KeyCode::Character && ascii.value == 'x');
    static_assert(high.code == sjtu::KeyCode::Character && high.value == 0xFFU);
}

void characterPredicatesCheckCodeAndValue() {
    constexpr auto x = sjtu::KeyEvent::character('x');
    constexpr sjtu::KeyEvent escape{sjtu::KeyCode::Escape, 'x'};

    static_assert(x.isCharacter('x'));
    static_assert(!x.isCharacter('X'));
    static_assert(!escape.isCharacter('x'));
}

void controlPredicateChecksCodeAndMaskedValue() {
    constexpr auto controlQ = sjtu::KeyEvent::character(sjtu::controlKey('q'));
    constexpr auto printableQ = sjtu::KeyEvent::character('q');
    constexpr sjtu::KeyEvent enter{sjtu::KeyCode::Enter, sjtu::controlKey('q')};

    static_assert(controlQ.isControl('q'));
    static_assert(controlQ.isControl('Q'));
    static_assert(!controlQ.isControl('p'));
    static_assert(!printableQ.isControl('q'));
    static_assert(!enter.isControl('q'));
}

void keyEventKeepsSimpleValueSemantics() {
    static_assert(std::is_trivially_copyable_v<sjtu::KeyEvent>);
    constexpr sjtu::KeyEvent defaultEvent{};
    static_assert(defaultEvent.code == sjtu::KeyCode::Character);
    static_assert(defaultEvent.value == 0U);
}

} // namespace

int main() {
    return test::run({
        {"controlKey uses the ASCII control mask", controlKeyUsesAsciiControlMask},
        {"character factory preserves every byte", characterFactoryPreservesEveryByte},
        {"isCharacter checks code and value", characterPredicatesCheckCodeAndValue},
        {"isControl checks code and masked value", controlPredicateChecksCodeAndMaskedValue},
        {"KeyEvent has simple value semantics", keyEventKeepsSimpleValueSemantics},
    });
}
