#include "DbDriver.hpp"

#if DB_RECORD_CACHE
#include "DbCache.hpp"
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define strlcpy(target, source) strncpy(target, source, (target).size() - 1)
#define strlcat(target, source) strncat(target, source, (target).size() - MIN((target).size() - 1, strlen(target)))

#define FilePath DbDriver::FilePath

#if USE_FF
FilePath tableDirPath = "/db";
#else
FilePath tableDirPath = "./db";
#endif


DbEventPublisher sCreateCallback = nullptr;
DbEventPublisher sDeleteCallback = nullptr;

// enough space for a max size serializeable + an appended commit id
thread_local uint8_t workBuffer[base_message::BodyMaxLength + sizeof(ObjId)] = {0};

void DbDriver::SetDirectory(const FilePath& path)
{
    tableDirPath = path;
}

uint8_t* DbDriver::WorkBuffer()
{
    return workBuffer;
}

#if DB_RECORD_CACHE
DbCache cache = {1024L * 1024L * 1L * 1024L}; // 1 Gb
#endif

void DbDriver::ClearCache()
{
#if DB_RECORD_CACHE
    cache.Clear();
#endif
}

FilePath DbDriver::IdToFileName(ObjId id)
{
    auto hex = binToHex<sizeof(id)>(&id);
    auto fp = FilePath();
    fp.Deserialize(hex.data());
    return fp;
}

#ifndef DARUMA_DB_RO
bool DbDriver::InitTable(const char* fullPath) const
{
    bool somethingFailed = false;

    if (DirectoryWrapper::New(fullPath))
    {
        LOG("Error creating table directory:");
        somethingFailed = true;
    }

    FilePath fp;
    strlcpy(fp, fullPath);
    strlcat(fp, "/counter");

    if (DirectoryWrapper::New(fp))
    {
        LOG("Error creating table counter directory:");
        somethingFailed = true;
    }

    strlcpy(fp, fullPath);
    strlcat(fp, "/pending");

    if (DirectoryWrapper::New(fp))
    {
        LOG("Error creating pending table directory:");
        somethingFailed = true;
    }

    return !somethingFailed;
}

bool DbDriver::InitDb()
{
    bool somethingFailed = false;

    if (!DirectoryWrapper::Exists(tableDirPath) && !DirectoryWrapper::New(tableDirPath))
    {
        LOG("Error creating db directory");
        somethingFailed = true;
    }

    return !somethingFailed;
}

bool DbDriver::InitScope(ObjId scope)
{
    if (scope == RootScope) return false;

    FilePath fp = ScopePath(scope);

    // FF doesn't like trailing slashes
    size_t end = strlen(fp) - 1;
    if ('/' == fp[end]) fp[end] = '\0';

    bool somethingFailed = false;
    if (!DirectoryWrapper::Exists(fp) && DirectoryWrapper::New(fp)) {
        somethingFailed = true;
    }

    return !somethingFailed;
}

bool DbDriver::SaveObjCt(const char * tableName, ObjId id)
{
    FilePath fp = TableNameToCounterPath(tableName);

    FileWrapper f{fp, "w"};
    if (!f.DidOpen()) {
        return false;
    }
    if (!f.Write(&id, sizeof(id))) {
        LOG("Error writing obj count file, bytes written not equal to bytes to write");
        return false;
    }

    return true;
}

#endif

ObjId DbDriver::GetObjCt(const char * tableName)
{
    FilePath fp = TableNameToCounterPath(tableName);
    FileWrapper file{fp, "r"};
    if (!file.DidOpen()) {
        return 1;
    }
    ObjId id;
    file.Read(&id, sizeof(ObjId));
    return id;
}

FilePath DbDriver::ScopePath(ObjId scope)
{
    FilePath fp = tableDirPath;
    strlcat(fp, "/");
    if (scope != RootScope) {
        strlcat(fp, binToHex<sizeof(mScope)>(&scope));
        strlcat(fp, "/");
    }

    return fp;
}

FilePath DbDriver::TableNameToCounterPath(const char* tableName)
{
    FilePath fp = ScopePath(mScope);
    strlcat(fp, tableName);

    char* p = ((char*)fp) + strlen(fp) - MIN(strlen(fp) - 1, strlen(tableName));
    for ( ; *p; ++p) *p = tolower(*p); // NOLINT

#ifndef DARUMA_DB_RO
    InitTable(fp);
#endif

    strlcat(fp, "/counter/objct");
    return fp;
}

FilePath DbDriver::TableNameToPath(const char* tableName)
{
    FilePath fp = ScopePath(mScope);

    strlcat(fp, tableName);

    char* p = ((char*)fp) + strlen(fp) - MIN(strlen(fp) - 1, strlen(tableName));
    for ( ; *p; ++p) *p = tolower(*p); // NOLINT

#ifndef DARUMA_DB_RO
    InitTable(fp);
#endif
    if (mPending) {
        strlcat(fp, "/pending");
    }

    return fp;
}

FilePath DbDriver::GetRecordPath(ObjId id, const char* tableName)
{
    FilePath fp = TableNameToPath(tableName);
    strlcat(fp, "/");
    FilePath idPath = IdToFileName(id);
    strlcat(fp, idPath);
    return fp;
}

uint32_t DbDriver::ReadRecord(const char * const fullPath, void * data)
{
    size_t len = 0;
#if DB_RECORD_CACHE
    if (cache.GetItem(fullPath, (uint8_t*)data, len)) {
        return len;
    }
#endif
    FileWrapper f(fullPath);

    if (!f.DidOpen()) {
        return 0;
    }

    // Read entire file
    uint32_t crc = 0;

    if (f.Size() < sizeof(crc) + sizeof(ObjId)) {
        LOG("ERROR: File at path");
        LOG(fullPath);
        LOG("has no CRC");
        LOG("File length:");
        logging::Print(f.Size());
        return 0;
    }

    len = f.Size() - sizeof(crc);

    if (!f.Read(data, len)) {
        return 0;
    }

    if (!f.Read(&crc, sizeof(crc))) {
        return 0;
    }

    bool crcMatches = crc == crc32(data, f.Size() - sizeof(crc) - sizeof(ObjId));

    if (!crcMatches) {
        LOG("CRC MISMATCH!");
        LOG(fullPath);
        return 0;
    }


#if DB_RECORD_CACHE
    cache.AddItem(fullPath, (uint8_t*)data, len);
#endif

    return len;
}

size_t DbDriver::GetRecord(void * data, const ObjId id, const char* tableName)
{
    FilePath recordPath = GetRecordPath(id, tableName);

    size_t recordLen;


    recordLen = ReadRecord(recordPath, data);

    return recordLen;
}

bool DbDriver::OpenTable(const char* tableName, DirectoryWrapper& dir)
{
    FilePath fp = TableNameToPath(tableName);
    return dir.Open(fp);
}

bool DbDriver::RecordExists(ObjId id, const char * tableName)
{
    FilePath recordPath = GetRecordPath(id, tableName);
    return DirectoryWrapper::Exists(recordPath);
}

bool DbDriver::NextRecordPath(FilePath& p, DirectoryWrapper& dir)
{
    while(true) {
        bool isDir;
        if (!dir.NextPath(p, isDir)) {
            return false;
        }

        if (isDir) continue;

        return true;
    }
}

bool DbDriver::GetNextRecord(void * data, DirectoryWrapper& dir)
{
    FilePath recordPath;
    if (!NextRecordPath(recordPath, dir)) {
        return false;
    }

    return ReadRecord(recordPath, data) > 0;
}

#ifndef DARUMA_DB_RO

bool DbDriver::NextId(const char * tableName, ObjId& id, bool increment)
{
    id = GetObjCt(tableName);
    if (increment && !SaveObjCt(tableName, id + 1)) {
        LOG("Failed to increment obj counter");
        return false;
    }

    return true;
}

bool DbDriver::SaveRecord(ObjId id, ObjId commitId, const void * data, uint32_t len, const char* tableName)
{
    // All pending records should have a non-zero commit id
    assert(!mPending || commitId);
    FilePath recordPath = DbDriver::GetRecordPath(id, tableName);
    FileWrapper f(recordPath, "w");

    if (!f.DidOpen()) {
        LOG("Error creating db object file at path:");
        LOG(recordPath);
        return false;
    }

    LOG("Opened file at path:");
    LOG(recordPath);

    // use the workbuffer so we deinitely have room for the commit id
    if (data != workBuffer) {
        memcpy(workBuffer, data, len);
    }

    uint32_t crc = crc32(workBuffer, len);

    memcpy(workBuffer + len, &commitId, sizeof(commitId));
    len += sizeof(ObjId);

    if (!f.Write(workBuffer, len)) {
        LOG("Error writing new db object at path:");
        LOG(recordPath);
        return false;
    }

    if (!f.Write(&crc, sizeof(crc))) {
        return false;
    }

    // Ensure the id is less than the current counter,
    // otherwise our counter needs to be updated
    if (id >= GetObjCt(tableName)) {
        if (!SaveObjCt(tableName, id + 1)) {
            LOG("Failed to increment obj counter");
            return false;
        }
    }


#if DB_RECORD_CACHE
    cache.AddItem(recordPath, (uint8_t*)data, len);
#endif
    return true;
}

void DbDriver::SendOnCreateCallbackEvent(const char *tableName, ObjId id)
{
    if(sCreateCallback)
    {
        auto len = GetRecord(workBuffer, id, tableName);
        sCreateCallback(workBuffer, len, mScope, tableName);
    }
}

bool DbDriver::DeleteRecord(const ObjId id, const char * tableName)
{
    FilePath record = GetRecordPath(id, tableName);
    if(sDeleteCallback && !mPending)
    {
        size_t len = GetRecord(WorkBuffer(), id, tableName);
        sDeleteCallback(WorkBuffer(), len, mScope, tableName);
    }
#if DB_RECORD_CACHE
    cache.RemoveItem(record);
#endif
    return DirectoryWrapper::Delete(record);
}

bool DbDriver::DeleteTable(const char * tableName)
{
    FilePath fp = TableNameToPath(tableName);
#if DB_RECORD_CACHE
    cache.Clear();
#endif
    return DirectoryWrapper::Delete(fp);
}

bool DbDriver::DeleteAll()
{
    DbDriver db{RootScope, 0};
    bool success =  db.DeleteScope();
    InitDb();

#if DB_RECORD_CACHE
    cache.Clear();
#endif
    return success;
}

bool DbDriver::DeleteScope()
{
    FilePath fp = ScopePath(mScope);
    DirectoryWrapper dbDir{fp};

    if (!dbDir.DidOpen()) {
        LOG("Error opening directory at path");
        LOG(fp);
        return false;
    }

    bool somethingFailed = false;

    while (true) {
        FilePath fp;
        bool isDir;
        if (!dbDir.NextPath(fp, isDir)) {
            break;
        }

        // we only care about directories
        if (!isDir) continue;

        somethingFailed = somethingFailed || !DirectoryWrapper::Delete(fp);
    }

#if DB_RECORD_CACHE
    cache.Clear();
#endif

    return !somethingFailed;
}

void DbDriver::SetOnCreateCallback(DbEventPublisher createCallback) {
    sCreateCallback = createCallback;
}


void DbDriver::SetOnDeleteCallback(DbEventPublisher deleteCallback) {
    sDeleteCallback = deleteCallback;
}
#endif
