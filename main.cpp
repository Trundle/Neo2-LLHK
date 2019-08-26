#define UNICODE
#define WIN32_LEAN_END_MEAN

extern "C" {
#include <windows.h>
#include <sal.h>
}

#include <array>
#include <iostream>


class Neo2Hook;

extern "C" LRESULT keyboard_hook(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam);


namespace {
    thread_local Neo2Hook *installed_hook;
}


constexpr bool is_ascii_alnum(const wchar_t ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
}


enum class KeyPress {
    UP, DOWN
};

class KeyPressInjector {
private:
    void inject_unicode_key(wchar_t key, KeyPress key_press) {
        INPUT input{.type = INPUT_KEYBOARD, .ki = {
                .wVk = 0,
                .wScan = key,
                .dwFlags = KEYEVENTF_UNICODE,
        }};
        if (key_press == KeyPress::UP) {
            input.ki.dwFlags |= KEYEVENTF_KEYUP;
        }
        const auto sent = SendInput(1, &input, sizeof(INPUT));
        if (sent != 1) {
            std::cerr << "Could not send all keys: 1 != " << sent << std::endl;
        }
    }

public:
    void inject_virtual_key(uint16_t virtual_key, KeyPress key_press) {
        INPUT input{.type = INPUT_KEYBOARD, .ki = {
                .wVk = static_cast<WORD>(virtual_key),
                .wScan = virtual_key,
        }};
        if (key_press == KeyPress::UP) {
            input.ki.dwFlags |= KEYEVENTF_KEYUP;
        }
        const auto sent = SendInput(1, &input, sizeof(INPUT));
        if (sent != 1) {
            std::cerr << "Could not send all keys: 1 != " << sent << std::endl;
        }
    }

    void inject_key(const wchar_t key, KeyPress key_press) {
        if (is_ascii_alnum(key)) {
            // The virtual key codes of ASCII letters are their uppercase ASCII value, hence there
            // is no need to go through the (presumably) more expensive unicode input layer (i.e.
            // KEYEVENTF_UNICODE).
            // Also there is a high chance that an ascii key is used together with some modifier
            // as a shortcut / accelerator and those don't seem to work with unicode input.
            const auto virtual_key = key <= '9'
                                     ? static_cast<uint8_t>(key)
                                     : static_cast<uint8_t>(key) & ~0x20u;
            inject_virtual_key(virtual_key, key_press);
        } else {
            inject_unicode_key(key, key_press);
        }
    }
};


constexpr bool is_shift_key(const KBDLLHOOKSTRUCT &key_press) {
    return key_press.vkCode == VK_SHIFT
           || key_press.vkCode == VK_LSHIFT
           || key_press.vkCode == VK_RSHIFT;
}

constexpr bool is_mod3_key(const KBDLLHOOKSTRUCT &key_press) {
    return key_press.vkCode == VK_CAPITAL || key_press.vkCode == VK_OEM_2;
}

constexpr bool is_mod4_key(const KBDLLHOOKSTRUCT &key_press) {
    return key_press.vkCode == VK_RMENU || key_press.vkCode == VK_OEM_102;
}

constexpr KeyPress key_press_from_llhook_wparam(const WPARAM wParam) {
    return wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN
           ? KeyPress::DOWN
           : KeyPress::UP;
}

enum class KeyAction {
    SUPPRESS,
    PASS_THROUGH,
};


struct Neo2State {
    bool shift_pressed;
    bool mod3_pressed;
    bool mod4_pressed;
};

class Layer {
public:
    virtual KeyAction handle_key(KeyPressInjector &key_press_injector,
                                 const KBDLLHOOKSTRUCT &key_press, KeyPress type) = 0;

    virtual ~Layer() {};
};

template<typename T>
struct Singleton {
    static T &instance() {
        static const std::unique_ptr<T> instance{new T{}};
        return *instance;
    }
};

// Convenience abstract class for implementing layers: first looks for a match in a static translation table
template<size_t N>
class StaticTranslationTable : public Layer {
    virtual KeyAction do_handle_key(KeyPressInjector &key_press_injector,
                                    const KBDLLHOOKSTRUCT &key_press, KeyPress type) = 0;

    KeyAction handle_key(KeyPressInjector &key_press_injector,
                         const KBDLLHOOKSTRUCT &key_press, KeyPress type) override {
        if (!(key_press.flags & LLKHF_EXTENDED)
            && key_press.scanCode > 0 && key_press.scanCode < translation_map_size()) {
            const auto translated = translate(key_press.scanCode);
            if (translated) {
                key_press_injector.inject_key(translated, type);
                return KeyAction::SUPPRESS;
            } else {
                return do_handle_key(key_press_injector, key_press, type);
            }
        } else {
            return do_handle_key(key_press_injector, key_press, type);
        }
    }

protected:
    explicit StaticTranslationTable(std::array<wchar_t, N> &&translation_table_)
            : translation_table(translation_table_) {}

    virtual size_t translation_map_size() const {
        return N;
    }

    virtual wchar_t translate(size_t index) const {
        return translation_table[index];
    }

private:
    // Scan code to Character
    std::array<wchar_t, N> translation_table;
};

class Layer1 : public StaticTranslationTable<64>, public Singleton<Layer1> {
public:
    Layer1() : StaticTranslationTable(
            {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '`', '\0', '\0',
             'x', 'v', 'l', 'c', 'w', 'k', 'h', 'g', 'f', 'q', L'ß', L'´', '\0', '\0', 'u', 'i',
             'a', 'e', 'o', 's', 'n', 'r', 't', 'd', 'y', '\0', '\0', '\0', L'ü', L'ö', L'ä', L'p',
             'z', 'b', 'm', ',', '.', 'j', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
            }) {
    }

    KeyAction do_handle_key(KeyPressInjector &key_press_injector,
                            const KBDLLHOOKSTRUCT &key_press, const KeyPress type) override {
        return KeyAction::PASS_THROUGH;
    }

    ~Layer1() override = default;
};

class Layer2 : public StaticTranslationTable<64>, public Singleton<Layer2> {
public:
    Layer2() : StaticTranslationTable(
            {'\0', '\0', L'°', L'§', L'ℓ', L'»', L'«', '$', L'€', L'„', L'“', L'”', L'—', '\0', '\0', '\0',
             'X', 'V', 'L', 'C', 'W', 'K', 'H', 'G', 'F', 'Q', L'ẞ', L'´', '\0', '\0', 'U', 'I',
             'A', 'E', 'O', 'S', 'N', 'R', 'T', 'D', 'Y', '\0', '\0', '\0', L'Ü', L'Ö', L'Ä', L'P',
             'Z', 'B', 'M', '\0', L'•', 'J', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
            }) {
    }

    KeyAction do_handle_key(KeyPressInjector &key_press_injector,
                            const KBDLLHOOKSTRUCT &key_press, const KeyPress type) override {
        return KeyAction::PASS_THROUGH;
    }

    ~Layer2() override = default;
};

class Layer3 : public StaticTranslationTable<64>, public Singleton<Layer3> {
public:
    Layer3() : StaticTranslationTable(
            {'\0', '\0', L'¹', L'²', L'³', L'›', L'‹', L'¢', L'¥', L'‚', L'‘', L'’', L'‐', '\0', '\0', '\0',
             L'…', '_', '[', ']', '^', '!', '<', '>', '=', '&', L'ẞ', L'´', '\0', '\0', '\\', '/',
             '{', '}', '*', '?', '(', ')', '-', ':', '@', '\0', '\0', '\0', '#', '$', '|', '~',
             '`', '+', '%', '"', '\'', ';', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
            }) {
    }

    KeyAction do_handle_key(KeyPressInjector &key_press_injector,
                            const KBDLLHOOKSTRUCT &key_press, const KeyPress type) override {
        return KeyAction::PASS_THROUGH;
    }

    ~Layer3() override = default;
};

class Layer4 : public Layer, public Singleton<Layer4> {
public:
    KeyAction handle_key(KeyPressInjector &key_press_injector,
                         const KBDLLHOOKSTRUCT &key_press, const KeyPress type) override {
        if (key_press.scanCode >= 0 && key_press.scanCode < translation_table.size()) {
            const auto virtual_key = translation_table[key_press.scanCode];
            if (virtual_key) {
                key_press_injector.inject_virtual_key(virtual_key, type);
                return KeyAction::SUPPRESS;
            }
        }
        return KeyAction::PASS_THROUGH;
    }

    ~Layer4() override = default;

private:
    const std::array<uint16_t, 64> translation_table{
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, VK_BACK, VK_UP, VK_DELETE, 0, 0, VK_NUMPAD7, VK_NUMPAD8,
            VK_NUMPAD9, 0, 0, 0, 0, 0, VK_HOME, VK_LEFT,
            VK_DOWN, VK_RIGHT, VK_END, 0, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, 0,
            0, 0, 0, 0, VK_ESCAPE, VK_TAB, 0, VK_RETURN,
            0, 0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, 0, 0, 0,
            0, VK_NUMPAD0, 0, 0, 0, 0, 0, 0,
    };
};

class ByPassLayer : public Layer, public Singleton<ByPassLayer> {
public:
    KeyAction handle_key(KeyPressInjector &key_press_injector,
                         const KBDLLHOOKSTRUCT &key_press, const KeyPress type) override {
        return KeyAction::PASS_THROUGH;
    }
};

class Neo2Hook {
public:
    static KeyAction handle_key(const KBDLLHOOKSTRUCT &key_press, const KeyPress type) {
        if (installed_hook == nullptr) {
            std::cerr << "Hook called, but no hook registered :o :o" << std::endl;
            exit(1);
        }
        return installed_hook->do_handle_key(key_press, type);
    }

    Neo2Hook() : key_press_injector(KeyPressInjector{}),
                 neo2_state({}),
                 layer(&Layer1::instance()),
                 bypass_requested(false) {}

    void Neo2Hook::run() {
        register_hook();

        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

private:
    KeyPressInjector key_press_injector;
    Neo2State neo2_state;
    Layer *layer;
    bool bypass_requested;

    bool handle_mod(const KBDLLHOOKSTRUCT &key_press, const KeyPress type, KeyAction &action) {
        const bool pressed = type == KeyPress::DOWN;
        if (pressed && key_press.vkCode == VK_END) {
            action = KeyAction::PASS_THROUGH;
            bypass_requested = !bypass_requested;
            return true;
        } else if (is_shift_key(key_press)) {
            neo2_state.shift_pressed = pressed;
            action = KeyAction::PASS_THROUGH;
            return true;
        } else if (is_mod3_key(key_press)) {
            neo2_state.mod3_pressed = pressed;
            action = KeyAction::SUPPRESS;
            return true;
        } else if (is_mod4_key(key_press)) {
            neo2_state.mod4_pressed = pressed;
            action = KeyAction::SUPPRESS;
            if (type == KeyPress::DOWN && key_press.vkCode == VK_RMENU) {
                // AltGr emits (at least on German keyboards) RMENU and LCONTROL together and we
                // can only suppress the RMENU press here (because they come as two individual
                // keypress events, but we don't want to suppress LCONTROL in general.
                // Hence simply inject up key event for the control press that already went through.
                key_press_injector.inject_virtual_key(VK_LCONTROL, KeyPress::UP);
            }
            return true;
        } else {
            return false;
        }
    }

    void determine_level() {
        if (bypass_requested) {
            layer = &ByPassLayer::instance();
        } else if (neo2_state.mod4_pressed) {
            layer = &Layer4::instance();
        } else if (neo2_state.mod3_pressed) {
            layer = &Layer3::instance();
        } else if (neo2_state.shift_pressed) {
            layer = &Layer2::instance();
        } else {
            layer = &Layer1::instance();
        }
    }

    KeyAction do_handle_key(const KBDLLHOOKSTRUCT &key_press, const KeyPress type) {
        if (key_press.flags & LLKHF_INJECTED) {
            return KeyAction::PASS_THROUGH;
        }

        auto action = KeyAction::PASS_THROUGH;
        if (handle_mod(key_press, type, action)) {
            determine_level();
            return action;
        } else if (neo2_state.mod4_pressed && key_press.vkCode == VK_LCONTROL) {
            // See the comment in handle_mod: if mod4 is pressed, further control key presses
            // are likely caused because AltGr is kept pressed. As some applications (such as
            // CLion) react to repeated control presses, suppress them.
            return KeyAction::SUPPRESS;
        } else {
            return layer->handle_key(key_press_injector, key_press, type);
        }
    }

    void register_hook() {
        if (installed_hook != nullptr) {
            std::cerr << "Hook already installed!" << std::endl;
            exit(1);
        }

        auto instance = GetModuleHandle(nullptr);
        if (!instance) {
            std::cerr << "Could not get module handle: " << GetLastError() << std::endl;
            exit(1);
        }
        if (!SetWindowsHookEx(WH_KEYBOARD_LL, reinterpret_cast<HOOKPROC>(keyboard_hook), instance, 0)) {
            std::cerr << "Could not register low-level keyboard hook: " << GetLastError() << std::endl;
            exit(1);
        }
        installed_hook = this;
    }
};

extern "C" LRESULT keyboard_hook(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT &event = *reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
        const auto type = key_press_from_llhook_wparam(wParam);
        if (Neo2Hook::handle_key(event, type) == KeyAction::SUPPRESS) {
            return 1;
        } else {
            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }
    } else {
        // MSDN says this case should always be passed through and not handled
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }
}


int main() {
    auto hook = Neo2Hook{};
    hook.run();

    return 0;
}