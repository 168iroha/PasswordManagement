#include "SQLiteStmt.h"
#include "SQLiteView.h"
#include <bit>
#include <iostream>
#include <stdexcept>

void SQLiteStmtControl::keep(std::size_t mask) noexcept {
    this->control |= mask;
}

void SQLiteStmtControl::dispose(std::size_t mask) noexcept {
    if (this->stmt != nullptr) {
        this->control &= ~mask;
        if (this->control == 0) {
            // 他で利用されていない場合でのみ開放(SQLITE_ERRORを返すとしてもエラーとは限らないため無視)
            sqlite3_finalize(this->stmt);
            this->stmt = nullptr;
        }
    }
}

SQLiteStmtControl::SQLiteStmtControl(std::shared_ptr<SQLiteConnection> conn, sqlite3_stmt& stmt, std::size_t control) : conn(conn), stmt(std::addressof(stmt)), control(control) {}

SQLiteStmtControl::~SQLiteStmtControl() {
    this->dispose(~0);
}

SQLiteStmtControl::SQLiteStmtControl(SQLiteStmtControl&& x) noexcept {
    *this = std::move(x);
}

SQLiteStmtControl& SQLiteStmtControl::operator=(SQLiteStmtControl&& x) noexcept {
    this->dispose(~0);
    this->conn = std::move(x.conn);
    this->stmt = x.stmt;
    this->control = x.control;
    x.stmt = nullptr;
    x.control = 0;
    return *this;
}

SQLiteStmt::SQLiteStmt(std::shared_ptr<SQLiteStmtControl> control) : _control(control) {
    this->_control->keep(SQLiteStmtControl::ENABLE_SQLITE_STMT);
}

SQLiteStmt::~SQLiteStmt() {
    this->_control->dispose(SQLiteStmtControl::ENABLE_SQLITE_STMT);
}

void SQLiteStmt::bind(int index, std::u8string_view data) {
    sqlite3_bind_text(this->_control->stmt, index, std::bit_cast<const char*>(data.data()), static_cast<int>(data.size()), SQLITE_STATIC);
}

void SQLiteStmt::bind(int index, const std::u8string& data) {
    sqlite3_bind_text(this->_control->stmt, index, std::bit_cast<const char*>(data.data()), -1, SQLITE_STATIC);
}

void SQLiteStmt::bind(int index, const std::chrono::utc_seconds& data) {
    // 明示的にコピーをバインドする
    sqlite3_bind_text(this->_control->stmt, index, std::format("{:%Y-%m-%d %H:%M:%S}", data).c_str(), -1, SQLITE_TRANSIENT);
}

void SQLiteStmt::bind(int index, nullptr_t) {
    sqlite3_bind_null(this->_control->stmt, index);
}

void SQLiteStmt::bind(int index, const std::vector<unsigned char>& data) {
    sqlite3_bind_blob64(this->_control->stmt, index, data.data(), static_cast<sqlite3_uint64>(data.size()), SQLITE_STATIC);
}

SQLiteView SQLiteStmt::exec() {
    if ((this->_control->control & (SQLiteStmtControl::ENABLE_SQLITE_VIEW | SQLiteStmtControl::ENABLE_SQLITE_ITERATOR)) != 0) {
        throw std::logic_error("有効なSQLiteViewあるいはSQLiteIteratorが存在しているためSQLiteViewを生成することはできません");
    }

    // SQLの実行状態をリセットする
    if (sqlite3_reset(this->_control->stmt) != SQLITE_OK) {
        throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(sqlite3_db_handle(this->_control->stmt)));
    }
    return SQLiteView(this->_control);
}

SQLiteStmt::SQLiteStmt(SQLiteStmt&& x) noexcept {
    *this = std::move(x);
}

SQLiteStmt& SQLiteStmt::operator=(SQLiteStmt&& x) noexcept {
    this->_control = std::move(x._control);
    return *this;
}
