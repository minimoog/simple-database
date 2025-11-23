#ifndef _CHILDTABLE_HPP_
#define _CHILDTABLE_HPP_

#include "Table.hpp"

/**
 * ChildTable template cass
 * Provides methods for finding and delting children
 * Inherit from this if you are a table and there is another table that references you
 * e.g. A UserTable might inherit from ChildTable<Permission>
 *
 * If you have a complicated relationship to your child, use the `customFilter` argument.
 * This is especially useful for polymorphic columns where a match may not necessarily indicate a child
 */
template <class T, class Table>
class ChildTable {
    public:
        /**
         * Constructor
         * @param scope - The db scope of the tble, should be the same as the parent
         * @param columnName - The name of the column that references the parent table
         * @param [customFilter] - Optional filter used in addition to the foreign key to find the children
         */
        ChildTable(ObjId scope, const char* columnName, std::function<bool(T*)> customFilter = nullptr) :
            mScope{scope},
            mCustomFilter(customFilter)
        {
            strcpy(mColumnName, columnName);
        }

        /**
         * Children - Find all the child records in the ChildTable
         * @param foreignKey - The id of the parent record
         */
        virtual ResultSet<T> Children(ObjId foreignKey);

        /**
         * DeleteChildren - Delete all the child records in the ChildTable
         * @param foreignKey - The id of the parent record
         */
        DbError DeleteChildren(ObjId foreignKey);

    protected:
        ObjId mScope;
        Table mTable{mScope};

    private:
        char mColumnName[BaseProperty::MaxPropertyNameLength];
        std::function<bool(T*)> mCustomFilter = nullptr;
};

template <class T, class Table>
ResultSet<T> ChildTable<T, Table>::Children(ObjId foreignKey)
{
    return mTable.CustomSearch(Query::ResultType::Many, [&foreignKey, this](uint8_t* data) {
        // deserialize the data for use in the custom filter
        T record;
        record.Deserialize(data, false);

        // Run the custom filter first if it exists
        if (mCustomFilter != nullptr && !mCustomFilter(&record)) {
            return false;
        }

        // Check if the foreignKey matches the specified column
        Query q = WhereNeedleQuery(mColumnName, &foreignKey, sizeof(ObjId), true);
        NeedleTest<T> test{record, q};
        return test(data);
    });
}

template <class T, class Table>
DbError ChildTable<T, Table>::DeleteChildren(ObjId foreignKey)
{
    auto results = Children(foreignKey);
    if (!results.success) {
        return ErrorCode::None;
    }
    return mTable.Delete(results);
}

#endif //_CHILDTABLE_HPP_
