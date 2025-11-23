#ifndef _CUSTOMCHILDTABLE_HPP_
#define _CUSTOMCHILDTABLE_HPP_
#include "ChildTable.hpp"

template <class T, class Table>
class CustomChildTable : public ChildTable<T, Table>
{
    public:
        CustomChildTable(ObjId scope, std::function<bool(T*, ObjId)> customFilter) :
            ChildTable<T, Table>{scope, ""}, mCustomFilter{customFilter} {}

        ResultSet<T> Children(ObjId foreignKey) override;
    private:
        std::function<bool(T*, ObjId)> mCustomFilter;
};

template <class T, class Table>
ResultSet<T> CustomChildTable<T, Table>::Children(ObjId foreignKey)
{
    return this->mTable.CustomSearch(Query::ResultType::Many, [&foreignKey, this](uint8_t* data) {
        T record;
        record.Deserialize(data, false);

        // Run the custom filter first if it exists
        return mCustomFilter(&record, foreignKey);
    });
}

#endif //_CUSTOMCHILDTABLE_HPP_
