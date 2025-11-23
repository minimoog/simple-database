#ifndef _TABLEMAP_HPP_
#define _TABLEMAP_HPP_

#include "MessageDefinitions.hpp"
%%MESSAGE_WITH_ID_LOOP%%
#include "$$MESSAGE_TYPE$$.hpp"
%%MESSAGE_WITH_ID_LOOP%%
%%CC_MESSAGE_WITH_ID_LOOP%%
#include "$$MESSAGE_TYPE$$.hpp"
%%CC_MESSAGE_WITH_ID_LOOP%%
%%CRYPTO_MESSAGE_WITH_ID_LOOP%%
#include "$$MESSAGE_TYPE$$.hpp"
%%CRYPTO_MESSAGE_WITH_ID_LOOP%%

namespace base_message {
    template <template <class T, class V=void> class Table, typename F>
    inline void MessageTypeToTable(MessageType m, uint64_t scope, bool pending, F callback) {
        switch (m) {
#if CRYPTOCURRENCY
            %%CRYPTO_MESSAGE_WITH_ID_LOOP%%
            case MessageType::$$MESSAGE_TYPE$$Type: {
                auto table = Table<$$MESSAGE_TYPE$$>{scope, pending};
                callback(&table);
                return;
            }
            %%CRYPTO_MESSAGE_WITH_ID_LOOP%%
#endif
#if CONFIDENTIAL_COMPUTE
            %%CC_MESSAGE_WITH_ID_LOOP%%
            case MessageType::$$MESSAGE_TYPE$$Type: {
                auto table = Table<$$MESSAGE_TYPE$$>{scope, pending};
                callback(&table);
                return;
            }
            %%CC_MESSAGE_WITH_ID_LOOP%%
#endif
            %%MESSAGE_WITH_ID_LOOP%%
            case MessageType::$$MESSAGE_TYPE$$Type: {
                auto table = Table<$$MESSAGE_TYPE$$>{scope, pending};
                callback(&table);
                return;
            }
            %%MESSAGE_WITH_ID_LOOP%%
            default: {
                assert(false);
                return;
            }
        }
    }

    inline static const MessageType TableTypes[] = {
#if CRYPTOCURRENCY
        %%CRYPTO_MESSAGE_WITH_ID_LOOP%%
        MessageType::$$MESSAGE_TYPE$$Type,
        %%CRYPTO_MESSAGE_WITH_ID_LOOP%%
#endif
#if CONFIDENTIAL_COMPUTE
        %%CC_MESSAGE_WITH_ID_LOOP%%
        MessageType::$$MESSAGE_TYPE$$Type,
        %%CC_MESSAGE_WITH_ID_LOOP%%
#endif
        %%MESSAGE_WITH_ID_LOOP%%
        MessageType::$$MESSAGE_TYPE$$Type,
        %%MESSAGE_WITH_ID_LOOP%%
    };
}

#endif //_TABLEMAP_HPP_
