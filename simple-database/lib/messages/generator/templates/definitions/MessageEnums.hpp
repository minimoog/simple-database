#ifndef _MESSAGEENUMS_HPP_
#define _MESSAGEENUMS_HPP_

#include <stdint.h>
#include <string.h>
#include "BaseEnumWrapper.hpp"

namespace base_message {
    class MessageTypeWrapper : public BaseEnumWrapper<MessageType> {
        public:
        MessageTypeWrapper() = default;
        MessageTypeWrapper(MessageType e) { value = e; }
        MessageTypeWrapper(uint32_t e) { value = (MessageType)e; }
        EnumString ToString() const override {
            return MessageTypeToString(value);
        }
        void FromString(const EnumString& str) override {
            value = MessageTypeFromString(str);
        }
    };


    %%ENUM_LOOP%%
    enum class $$ENUM_NAME$$ : uint32_t {
        %%ENUM_VALUE_LOOP%%
        $$ENUM_VALUE$$,
        %%ENUM_VALUE_LOOP%%
    };

    constexpr std::array<$$ENUM_NAME$$, $$ENUM_SIZE$$> All$$ENUM_NAME$$ = {
        %%ENUM_VALUE_LOOP%%
        $$ENUM_NAME$$::$$ENUM_VALUE$$,
        %%ENUM_VALUE_LOOP%%
    };

    inline EnumString $$ENUM_NAME$$ToString($$ENUM_NAME$$ e)
    {
        switch(e) {
            %%ENUM_VALUE_LOOP%%
            case $$ENUM_NAME$$::$$ENUM_VALUE$$:
                return "$$ENUM_VALUE$$";
            %%ENUM_VALUE_LOOP%%
            default:
                return "";
        }
    }

    inline $$ENUM_NAME$$ $$ENUM_NAME$$FromString(EnumString str)
    {
        %%ENUM_VALUE_LOOP%%
        if (str == "$$ENUM_VALUE$$") {
            return $$ENUM_NAME$$::$$ENUM_VALUE$$;
        }
        %%ENUM_VALUE_LOOP%%

        return ($$ENUM_NAME$$)0;
    }

    class $$ENUM_NAME$$Wrapper : public BaseEnumWrapper<$$ENUM_NAME$$> {
        public:
            $$ENUM_NAME$$Wrapper() = default;
            $$ENUM_NAME$$Wrapper($$ENUM_NAME$$ e) { value = e; }
            $$ENUM_NAME$$Wrapper(uint32_t e) { value = ($$ENUM_NAME$$)e; }
            EnumString ToString() const override {
                return $$ENUM_NAME$$ToString(value);
            }
            void FromString(const EnumString& str) override {
                value = $$ENUM_NAME$$FromString(str);
            }
    };
    %%ENUM_LOOP%%
};

#endif //_MESSAGEENUMS_HPP_
