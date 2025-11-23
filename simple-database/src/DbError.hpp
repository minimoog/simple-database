#ifndef _DBERROR_HPP_
#define _DBERROR_HPP_

#include "FixedLengthString.hpp"
#include "MessageEnums.hpp"

using DbErrorString = FixedLengthString<255>;
class DbError {
    public:
        DbError() = default;
        DbError(const base_message::ErrorCode& code, const DbErrorString& details = "") :
            mCode{code},
            mDetails{details} {}

        inline operator bool() const {
            return IsError();
        }

        bool IsError() const {
            return mCode != base_message::ErrorCode::None;
        }

        friend inline bool operator==(const DbError& err, const base_message::ErrorCode& code) {
            return err.mCode == code;
        }

        friend inline bool operator==(const base_message::ErrorCode& code, const DbError& err) {
            return err == code;
        }

        friend inline bool operator!=(const DbError& err, const base_message::ErrorCode& code) {
            return !(err == code);
        }

        friend inline bool operator!=(const base_message::ErrorCode& code, const DbError& err) {
            return !(err == code);
        }

        base_message::ErrorCode Code() const { return mCode; };

        operator base_message::ErrorCode() const {
            return Code();
        }

        const DbErrorString& Details() const { return mDetails; };
    private:
        base_message::ErrorCode mCode = base_message::ErrorCode::None;
        DbErrorString mDetails;
};
#endif //_DBERROR_HPP_
