#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <sys/stat.h>

using ::testing::Eq;
using ::testing::Test;

const auto kPath = "/tmp/sqlitetest";
const auto kJournalPath = "/tmp/sqlitetest-journal";
const auto kIntegrityCheck = "pragma integrity_check;";
const auto kSchema = "create table if not exists t (i text unique);";
const auto kInsert = "insert into t (i) values ('abc');";
const auto kSelect = "select * from t;";
const auto kDelete = "delete from t;";

class ADbWithEmptyJournalFolder : public Test {
protected:
    sqlite3* db;
    void SetUp() override
    {
        system((std::string("rm -rf ") + kPath).c_str());
        system((std::string("rm -rf ") + kJournalPath).c_str());
        system((std::string("mkdir -p ") + kJournalPath).c_str());
    }
};

TEST_F(ADbWithEmptyJournalFolder, ReturnsCantopen14orError1orOkIfOpen)
{
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_CANTOPEN));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_ERROR));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_ERROR));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_ERROR));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithEmptyJournalFolder, IsEmptyIfOpen)
{
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_CANTOPEN));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));

    struct stat stat_buf;
    EXPECT_THAT(stat(kPath, &stat_buf), Eq(0));
    EXPECT_THAT(stat_buf.st_size, Eq(0));
}

TEST_F(ADbWithEmptyJournalFolder, ReturnsCantopen14orError1orOkIfOpenEmpty)
{
    system((std::string("> ") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_CANTOPEN));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_ERROR));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_ERROR));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_ERROR));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithEmptyJournalFolder, ReturnsIoerr10IfOpenCorrupt)
{
    system((std::string("echo trash > ") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_IOERR));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithEmptyJournalFolder, StatementReturnsMisuse21IfOpenCorrupt)
{
    system((std::string("echo trash > ") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, kIntegrityCheck, -1, &stmt, 0);
    EXPECT_THAT(sqlite3_step(stmt), Eq(SQLITE_MISUSE));
    sqlite3_finalize(stmt);
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}
