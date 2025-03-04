#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sqlite3.h>

using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Test;

const auto kPath = "/tmp/sqlitetest";
const auto kJournalPath = "/tmp/sqlitetest-journal";
const auto kBackupPath = "/tmp/sqlitetest-backup";
const auto kIntegrityCheck = "pragma integrity_check;";
const auto kSchema = "create table if not exists t (i text unique);";
const auto kInsert = "insert into t (i) values ('abc');";
const auto kSelect = "select * from t;";
const auto kDelete = "delete from t;";

class ADbWithBackup : public Test {
protected:
    sqlite3* db;
    sqlite3* backup;
    sqlite3_backup* op;
    void SetUp() override
    {
        system((std::string("rm -rf ") + kPath).c_str());
        system((std::string("rm -rf ") + kJournalPath).c_str());
        system((std::string("rm -rf ") + kBackupPath).c_str());
    }
};

TEST_F(ADbWithBackup, WorksIfOpenWithEmptyBackup)
{
    system((std::string("> ") + kBackupPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_DONE));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsNotadb26IfOpenWithCorruptBackup)
{
    system((std::string("echo trash > ") + kBackupPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_NOTADB));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_NOTADB));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, WorksIfOpenWithBackup)
{
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(backup, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(backup, kInsert, 0, 0, 0), Eq(SQLITE_OK));

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_DONE));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsNotadb26IfOpenWithBackupCorrupt)
{
    system((std::string("echo trash > ") + kPath).c_str());
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(backup, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(backup, kInsert, 0, 0, 0), Eq(SQLITE_OK));

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_NOTADB));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_NOTADB));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsCorrupt11IfOpenWithPartiallyCorruptBackup)
{
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(backup, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(backup, kInsert, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kBackupPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsCorrupt11IfOpenWithEmptyBackupPartiallyCorrupt)
{
    system((std::string("> ") + kBackupPath).c_str());
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsNotadb26IfOpenWithCorruptBackupPartiallyCorrupt)
{
    system((std::string("echo trash > ") + kBackupPath).c_str());
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_NOTADB));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_NOTADB));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsCorrupt11IfOpenWithBackupPartiallyCorrupt)
{
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
    system((std::string("cp ") + kPath + " " + kBackupPath).c_str());
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, ReturnsCorrupt11IfOpenWithPartiallyCorruptBackupPartiallyCorrupt)
{
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
    system((std::string("cp ") + kPath + " " + kBackupPath).c_str());
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kPath).c_str());
    system((std::string("dd if=/dev/urandom seek=2 count=1 of=") + kBackupPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_CORRUPT));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}

TEST_F(ADbWithBackup, WorksIfOpenExistingWithBackup)
{
    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSchema, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kInsert, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
    system((std::string("cp ") + kPath + " " + kBackupPath).c_str());

    ASSERT_THAT(sqlite3_open(kPath, &db), Eq(SQLITE_OK));
    ASSERT_THAT(sqlite3_open(kBackupPath, &backup), Eq(SQLITE_OK));
    op = sqlite3_backup_init(db, "main", backup, "main");
    ASSERT_THAT(op, NotNull());
    EXPECT_THAT(sqlite3_backup_step(op, -1), Eq(SQLITE_DONE));
    EXPECT_THAT(sqlite3_backup_finish(op), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(backup), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kIntegrityCheck, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kSelect, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_exec(db, kDelete, 0, 0, 0), Eq(SQLITE_OK));
    EXPECT_THAT(sqlite3_close_v2(db), Eq(SQLITE_OK));
}
