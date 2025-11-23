# Simple DB
Has anyone really been far even as decided to use simple DB?

Scientists may never know the answer to this and such other questions

But what we _can_ know, is how to use it

### Configuring & Building

There is a single option that can modify how whether the db uses the FF FatFS
library as its backend or the native OS filesystem (using std::filesystem & cstdio)

```
USE_FF=0
```
To define this at compile time just add the define to the cmake build command:
```bash
cd build && cmake -DUSE_FF=1 ..
```
If you don't specify the option then it will default to using the native filesystem

### What lives in the db and how do I interact with it?
The db holds any `Serializeable` you want, provided it has an `uint64_t` `Id`
property.

Just construct a table like this:
```c++
Table<MyObject> objectTable;
```
...and start using it
```c++
MyObject myObj;
objectTable.Save(myObj);
```

#### Ids
Saving an object that doesn't have an Id will provision an id based on an internal
counter for the table.

There are no checks for overwriting existing records if you use the same ID,
so you probably shouldn't set the Id field, unless you are intending to overwrite
something.

For most case, let the DB handle Ids for you.

The database keeps an internal counter for each table, which will get incremented
whenever an Id is provisioned. If any record is saved with an Id higher than the
counter, the counter will move to the record's set Id.

##### Id peeking
It is possible to "peek" at the next id that will be provisioned, using the method
`PeekNextId`. This is useful if you _might_ be creating a record that needs to
reference another table, but the parent record shouldn't be created unless it has
a single child.

Be very careful when using `PeekNextId`, as this method does *not* reserve the Id.
If any record gets saved without an Id being set, it will take the peeked id.

Note: _Ids start at 1_

#### Basics
Saving/finding/delteing is pretty self explanitory.
I'll leave it as an exercise to the reader to look at `Table.hpp` and figure
out the basics

A little less obvious is how the `ResultSet` works that is returned by some of the
functions in `Table`.

#### ResultSets
A result set doesn't actually hold any data, just the ids that matched the query.
In order to save memory it's _your_ job to fetch each result one by one.
Luckily, that's pretty easy:
```c++
// First, lets get a reference to the `MyObject` where the data will be deserialized
MyObject& obj = objectTable.LoadedRecord();

// Get all the `MyObjects` in the db
ResultSet<MyObject> results = objectTable.All();

// success will be `true` if there is at least one item
if (results.success) { 
    while(true) {
        if (!objectTable.LoadNextResult(results)) { // this increments the results
            // but if there aren't any more results it wil return `false`
            std::cout << "No more results" << std::endl;
            break;
        }

        std::cout << obj.CoolProperty() << std::endl;
    }
}
```

#### Exact Match
A lot of the query functions on `Table` have the optional argument `exactMatch = true`.

Being an "exact" match means that the entire thing matches. e.g.
```c++
// this will find the first user with the name 'Elliot'
userTable.FindBy("Name", "Elliot");

// but it won't find anyone called 'Elliot Alderson'
// to find them too we would want to set `exactMatch` to `false`
userTable.FindBy("Name", "Elliot", false);

// `exactMatch == false` requires that the start is the same
// The above query won't find a user named "Mr Robot (AKA Elliot Alderson)"
```

#### Not
To negate a query, call `#Not()` on the table before the query:
```c++
// e.g. "Find all the people who's names don't start with 'A'"
auto results = userTable.Not().Where("Name", "A", false);
```
#### Count
When counting records the result set returned has a `GetCount`
method that can be used to show the count. If there were no results `(count == 0)`
then `success` will be set to `false`

#### Custom Search
What if you want to do something more complicated?

Lets say you wanted to find all the `Users` who's names are "Linux Tarballs"
and have a public key that starts with the bytes `0xDEADBEEF`.
Since this library doesn't implement a way to && together conditions how can we do that?

The `CustomSearch` exists so that you can easily add any test you want without the
library getting too complicated.

It takes in anything that will convert to `std::function<bool(T*)>` as an argument
(where `T` is the record type) and uses this to check which records should be returned.

Let's take a look
```c++
ResultSet<User> results = uTable.CustomSearch(Query::ResultType::Many, [](User* u) {
    uint32_t deadBeef = 0xDEADBEEF;
    return 0 == memcmp(u->PublicKey(), &deadBeef, sizeof(deadBeef)) &&
        0 == strcmp(u->Name(), "Linux Tarballs");
});
```

The results returned will either behave the same as when performing a `Where` or a
`Count` operation

_It is also possible to pass in a function that receives the records in serialized
form as a `uint8_t*`. Just pass in something that converts to `std::function<bool(uint8_t*)>`
instead_

#### Validations
A Table can be initialized with a validator class that can be used to check records
before they are saved.
This is useful for enforcing uniqueness, making sure fields don't get saved as zero
accidentally and making sure foreign keys actually correspond to something.

If you want to add validations for a class simply extend the validator, and implement
the `RecordIsValid` method:
```c++
class DogValidator : Validator<Dog> {
    // `Dog` is the corresponding Serializeable we are validating
    public:
        DogValidator(ObjId scope) : Validator<Dog>(scope) {}
        bool RecordIsValid(Dog& record) override;
}

// Using these macros makes adding a new validation easy
// Just make sure you name the argument `record` or they wont' work
bool DogValidator::RecordIsValid(Dog& record)
{
    // Make sure its not a stray!
    NOT_ZERO_VALIDATION("OwnerId", record.OwnerId());

    // Make sure the owner really exists
    // This checks that the `Table<Owner>` has an owner with that id (unless it is zero)
    FOREIGN_KEY_VALIDATION(Owner, record.OwnerId());

    // Don't forget to name it
    NOT_EMPTY_VALIDATION("Name", record.Name());

    // Also try and be original about it
    // This checks that the `Table<Dog>` doesn't have another dog by that name
    UNIQUE_VALIDATION(Dog, "Name", record.Name());

    return true;
}

```

Then instantiate the table with your validator:
```c++
Table<Dog, DogValidator> table;
if (!table.save(goodDogo)) {
    std::cout << "Oh no, this dogo is bad actually" << std::endl;
}
```

#### Scope
A `Table` or `DbDriver` can be initialized with a scope. This scope is an `ObjId`
(`uint64_t`), and controls the root directory of the `/db` folder that will be used
for all operations. e.g. If the scope is 1, all the data will be saved and searched
within the `/db/0100000000000000/` directory.

The intended use of this feature is for scoping the data that belongs to an organization
to that organization. This makes it much easier to perform searches & bulk deletions
within a specific organization, and should make backing up the data of that organization
pretty easy too.

The `RootScope` is the `0` id, and will use the root `/db` directory. Be aware that
calling `DbDriver db{0}; db.DeleteScope();` is the same as calling `DbDriver::DeleteAll()`


#### ChildTable
It may be useful to create specializations of the `Table` template for more complicated
records. Particularly if there are other tables that reference it. The `ChildTable<T>`
template allows a table to identify which tables are it's children, and then they
can easily be found & deleted.

e.g.
```c++
class UserTable : public Table<User, UserValidator>,
    public ChildTable<VaultPermission>,
    public ChildTable<OrganizationPermission>
{
    public:
        UserTable(ObjId scope) :
            Table<User, UserValidator>{scope},
            ChildTable<VaultPermission>{scope, "UserId"},
            ChildTable<OrganizationPermission>{scope, "UserId"} {}

        bool Delete(ObjId id) override
        {
            if (!Table<User, UserValidator>::Delete(id)) {
                return false;
            }

            if (!ChildTable<VaultPermission>::DeleteChildren(id)) {
                return false;
            }

            if (!ChildTable<OrganizationPermission>::DeleteChildren(id)) {
                return false;
            }

            return true;
        }
};
```

In situations where the relationship between parent & child is more complex, (i.e.
not merely defined by a single column), it is possible to pass in a `std::function<bool(T*)>`
to the `ChildTable<T>` constructor which will be used to test if the record is a child,
in addition to checking the foreign key.

This is useful for polymorphic columns where a match in the foreign key doesn't
necessarily indicate a child.

E.g.
```c++
class GoodDogOwner : public Table<User>,
    public ChildTable<Dog>
{
    public:
        GoodDogOwner(ObjId scope) :
            Table<User>{scope},
            ChildTable<Dog>{scope, "UserId", [](Dog* doggo) {
                // I only own good doggos
                return doggo->IsGood() == true;
            }} {}
};
```

#### DB Commits

A `Table` may be created in "Pre-Commit" mode, by passing a `preCommitId` to it's
constructor. When this happens all records will be written to a "pending" version
of the table, and have the `preCommitId` appended to the end of the record's data.

Any queries made on the table will only show data from the "pending" version.

Data with the specific `preCommitId` of the table can be viewed using the method
`AllWithCommitId`.

Data can be moved from the pending version to the main version using the `Commit`
or `CommitAll` methods.

Pending data will be validated when first created, and then again when committed.
It is also possible to validate all pending writes belonging to the `preCommitId`
using `ValidateCommit`. This allows the entire commit to be validated before any
transfer between the two tables begins.

Unique validations made using the `UNIQUE_VALIDATION` macro will run against both
the pending and main version of the `Table`.

