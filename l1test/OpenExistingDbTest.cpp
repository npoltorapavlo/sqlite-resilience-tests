#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sqlite3.h>

using ::testing::Eq;
using ::testing::Test;

const auto kPath = "/tmp/sqlitetest";
const auto kJournalPath = "/tmp/sqlitetest-journal";
const auto kIntegrityCheck = "pragma integrity_check;";
const auto kSchema = "create table if not exists t (i text unique);";
const auto kInsert = "insert into t (i) values ('abc');";
const auto kSelect = "select * from t;";
const auto kDelete = "delete from t;";

class AnExistingDb : public Test {
protected:
    sqlite3* db;
    void SetUp() override
    {
        system((std::string("rm -rf ") + kPath).c_str());
        system((std::string("rm -rf ") + kJournalPath).c_str());
        ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
        ASSERT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
        EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_OK));
        ASSERT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
    }
};

TEST_F(AnExistingDb, ReturnsCorrupt11IfOpenPartiallyCorrupt)
{
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(AnExistingDb, ReturnsIoerr10IfOpenPartiallyCorruptWithEmptyJournalFolder)
{
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());
    system((std::string("mkdir -p ") + kJournalPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(AnExistingDb, StatementReturnsMisuse21IfOpenPartiallyCorruptWithEmptyJournalFolder)
{
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());
    system((std::string("mkdir -p ") + kJournalPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, kIntegrityCheck, -1, &stmt, 0);
    EXPECT_THAT(sqlite3_step(stmt), Eq(SQLITE_MISUSE));
    sqlite3_finalize(stmt);
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(AnExistingDb, WorksIfOpenWithEmptyJournalFile)
{
    system((std::string("touch ") + kJournalPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_CONSTRAINT));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(AnExistingDb, WorksIfOpenWithCorruptJournalFile)
{
    system((std::string("echo trash > ") + kJournalPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_CONSTRAINT));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(AnExistingDb, ReturnsIoerr10IfOpenWithEmptyJournalFolder)
{
    system((std::string("mkdir -p ") + kJournalPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}
