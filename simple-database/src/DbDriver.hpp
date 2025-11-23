#ifndef _DBDRIVER_HPP_
#define _DBDRIVER_HPP_

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "crc.h"
#include "FileWrapper.hpp"
#include "DirectoryWrapper.hpp"
#include "logging.hpp"
#include "BaseMessageDefinitions.hpp"
#include "FixedLengthString.hpp"
#include <functional>

typedef uint64_t ObjId;
using DbEventPublisher = std::function<void(const void *recordData, uint32_t dataLength, ObjId scope, const char *tableName)>;

class DbDriver {
    public:

        static const ObjId RootScope = 0;
        using FilePath = FixedLengthString<PATH_MAX>;

        static uint8_t* WorkBuffer();
        static void SetDirectory(const FilePath& path);

        template<size_t inputLength>
        static FixedLengthString<inputLength * 2> binToHex(const void *bin) {
            auto hexOut = FixedLengthString<inputLength * 2>{};
            const char *hex = "0123456789abcdef";
            for (size_t i = 0; i < inputLength; i++) {
                hexOut[i * 2]     = hex[(((uint8_t*)bin)[i] >> 4) & 0xF];
                hexOut[i * 2 + 1] = hex[((uint8_t*)bin)[i] & 0xF];
            }
            hexOut[inputLength * 2] = '\0';

            return hexOut;
        }

        void SendOnCreateCallbackEvent(const char *tableName, ObjId id);

        DbDriver(ObjId scope, bool pending) : mScope(scope), mPending(pending) {
#ifndef DARUMA_DB_RO
            InitScope(scope);
#endif
        }

        size_t GetRecord(void * data, ObjId id, const char* tableName);
        bool OpenTable(const char* tableName, DirectoryWrapper& dir);
        bool GetNextRecord(void * data, DirectoryWrapper& dir);
        bool RecordExists(ObjId id, const char * tableName);

#ifndef DARUMA_DB_RO
        static bool InitDb();
        static bool DeleteAll();

        bool SaveRecord(ObjId id, ObjId commitId, const void * data, uint32_t len, const char* tableName);
        bool DeleteRecord(ObjId id, const char * tableName);
        bool NextId(const char * tableName, ObjId& id, bool increment = true);
        bool DeleteTable(const char * tableName);
        bool DeleteScope();
        static void SetOnCreateCallback(DbEventPublisher createCallback);
        static void SetOnDeleteCallback(DbEventPublisher deleteCallback);
#endif

        static void ClearCache();

    private:
        static FilePath IdToFileName(ObjId id);
        static FilePath ScopePath(ObjId scope);

        FilePath GetRecordPath(ObjId id, const char* tableName);

        FilePath TableNameToPath(const char* tableName);
        FilePath TableNameToCounterPath(const char* tableName);

#ifndef DARUMA_DB_RO
        bool InitScope(ObjId scope);
        bool InitTable(const char* fullPath) const;
        bool SaveObjCt(const char * tableName, ObjId id);
#endif
        ObjId GetObjCt(const char * tableName);
        uint32_t ReadRecord(const char * fullPath, void * data);
        bool NextRecordPath(FilePath& p, DirectoryWrapper& dir);
        ObjId mScope;
        bool mPending;
};

#endif //_DBDRIVER_HPP_
