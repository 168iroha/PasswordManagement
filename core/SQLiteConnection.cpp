#include "SQLiteConnection.h"
#include "SQLiteStmt.h"
#include <bit>
#include <iostream>
#include <stdexcept>

SQLiteConnection::~SQLiteConnection() {
    try {
        this->disconnect();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
}

void SQLiteConnection::connect(const std::filesystem::path& path) {
    this->disconnect();

    if (sqlite3_open(
        // パスはUTF8である必要がある
        reinterpret_cast<const char*>(path.u8string().data()),
        &this->conn
    ) != SQLITE_OK) {
        this->disconnect();
        throw std::runtime_error("SQLiteとの接続の確立に失敗");
    }
}

void SQLiteConnection::disconnect() {
    if (this->conn != nullptr) {
        if (sqlite3_close(this->conn) != SQLITE_OK) {
            this->conn = nullptr;
            throw std::runtime_error("SQLiteとの接続の切断に失敗");
        }
        this->conn = nullptr;
    }
}

SQLite::SQLite(const std::filesystem::path& path) : _conn(new SQLiteConnection) {
    this->_conn->connect(path);
}

void SQLite::exec(const std::u8string& sql) {
    char* errMsg = nullptr;
    // SQLを実行
    if (sqlite3_exec(
        this->_conn->conn,
        std::bit_cast<const char*>(sql.data()),
        nullptr,
        nullptr,
        &errMsg
    ) != SQLITE_OK) {
        std::string errMsg2 = std::string("SQL error: ") + errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error(errMsg2);
    }
}

SQLiteStmt SQLite::prepare(const std::u8string& sql) {
    sqlite3_stmt* stmt = nullptr;
    // プリペアドステートメントを作成
    if (sqlite3_prepare_v2(
        this->_conn->conn,
        std::bit_cast<const char*>(sql.data()),
        -1,
        &stmt,
        nullptr) != SQLITE_OK) {
        throw std::logic_error(std::string("SQL error: ") + sqlite3_errmsg(this->_conn->conn));
    }
    return SQLiteStmt(std::shared_ptr<SQLiteStmtControl>(new SQLiteStmtControl(this->_conn, *stmt, 0)));
}
