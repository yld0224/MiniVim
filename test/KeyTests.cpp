#include "Key.hpp"

#include "TestSupport.hpp"

#include <type_traits>

namespace {

void ControlKeyUsesTheAsciiMask() {
    static_assert(noexcept(sjtu::ControlKey('q')));
    static_assert(sjtu::ControlKey('@') == 0U);
    static_assert(sjtu::ControlKey('A') == 1U);
    static_assert(sjtu::ControlKey('q') == 17U);
    static_assert(sjtu::ControlKey('[') == 27U);
    static_assert(sjtu::ControlKey('?') == 31U);
    CHECK_EQ(sjtu::ControlKey('a'), sjtu::ControlKey('A'));
}

void CharacterFactoryPreservesTheByte() {
    constexpr auto ascii = sjtu::KeyEvent::Character('x');
    constexpr auto high = sjtu::KeyEvent::Character(0xFFU);
    static_assert(ascii.code_ == sjtu::KeyCode::Character);
    static_assert(ascii.value_ == 'x');
    static_assert(high.value_ == 0xFFU);
}

void PredicatesCheckBothCodeAndValue() {
    constexpr auto x = sjtu::KeyEvent::Character('x');
    constexpr auto control_q =
        sjtu::KeyEvent::Character(sjtu::ControlKey('q'));
    constexpr sjtu::KeyEvent escape{sjtu::KeyCode::Escape, 'x'};

    static_assert(x.IsCharacter('x'));
    static_assert(!x.IsCharacter('X'));
    static_assert(!escape.IsCharacter('x'));
    static_assert(control_q.IsControl('q'));
    static_assert(control_q.IsControl('Q'));
    static_assert(!control_q.IsControl('p'));
    static_assert(!escape.IsControl('q'));
}

void DefaultEventHasValueSemantics() {
    static_assert(std::is_trivially_copyable_v<sjtu::KeyEvent>);
    constexpr sjtu::KeyEvent event{};
    static_assert(event.code_ == sjtu::KeyCode::Character);
    static_assert(event.value_ == 0U);
}

} // namespace

int main() {
    return test::Run({
        {"ControlKey uses the ASCII control mask", ControlKeyUsesTheAsciiMask},
        {"Character preserves the input byte", CharacterFactoryPreservesTheByte},
        {"KeyEvent predicates check code and value", PredicatesCheckBothCodeAndValue},
        {"KeyEvent has simple default value semantics", DefaultEventHasValueSemantics},
    });
}
