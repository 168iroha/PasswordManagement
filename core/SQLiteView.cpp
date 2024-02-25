#include "SQLiteView.h"
#include <bit>
#include <iostream>
#include <stdexcept>

SQLiteData::SQLiteData(sqlite3_stmt& stmt) : _stmt(std::addressof(stmt)) {}

template <>
std::optional<SQLiteData::string_type> SQLiteData::get<SQLiteData::string_type>(int col) {
	int maxCols = sqlite3_column_count(this->_stmt);
	if (col >= maxCols) {
		throw std::invalid_argument(
			std::format("{0}番目のカラムは存在しません。カラムの最大数は{1}です", col, maxCols)
		);
	}

	switch (sqlite3_column_type(this->_stmt, col)) {
	case SQLITE_TEXT:
	{
		const unsigned char* p = sqlite3_column_text(this->_stmt, col);
		int len = sqlite3_column_bytes(this->_stmt, col);
		return SQLiteData::string_type{ std::bit_cast<const char8_t*>(p), static_cast<SQLiteData::string_type::size_type>(len) };
	}
	case SQLITE_NULL:
		return std::nullopt;
	}
	auto colstr = std::to_string(col);
	throw std::invalid_argument(
		std::format("{0}番目のカラムの型はTEXTもしくはNULLではありません。{0}番目のカラムの型は{1}です",
			colstr,
			sqlite3_column_decltype(this->_stmt, col)
		)
	);
}
template <>
std::optional<SQLiteData::blob_type> SQLiteData::get<SQLiteData::blob_type>(int col) {
	int maxCols = sqlite3_column_count(this->_stmt);
	if (col >= maxCols) {
		throw std::invalid_argument(
			std::format("{0}番目のカラムは存在しません。カラムの最大数は{1}です", col, maxCols)
		);
	}

	switch (sqlite3_column_type(this->_stmt, col)) {
	case SQLITE_BLOB:
	{
		const void* p = sqlite3_column_blob(this->_stmt, col);
		int len = sqlite3_column_bytes(this->_stmt, col);
		return SQLiteData::blob_type{ std::bit_cast<unsigned char*>(p), static_cast<SQLiteData::blob_type::size_type>(len) };
	}
	case SQLITE_NULL:
		return std::nullopt;
	}
	auto colstr = std::to_string(col);
	throw std::invalid_argument(
		std::format("{0}番目のカラムの型はTEXTもしくはNULLではありません。{0}番目のカラムの型は{1}です",
			colstr,
			sqlite3_column_decltype(this->_stmt, col)
		)
	);
}

SQLiteIterator::SQLiteIterator(std::shared_ptr<SQLiteStmtControl> control, int prevStep) : _control(control), _prevStep(prevStep) {
	this->_control->keep(SQLiteStmtControl::ENABLE_SQLITE_ITERATOR);
}

SQLiteIterator::~SQLiteIterator() {
	this->_control->dispose(SQLiteStmtControl::ENABLE_SQLITE_ITERATOR);
}

SQLiteData SQLiteIterator::operator*() const {
	return SQLiteData(*this->_control->stmt);
}

SQLiteIterator& SQLiteIterator::operator++() {
	if (this->_prevStep == SQLITE_ROW) {
		// 走査が終了していないときに次の行を取得する
		this->_prevStep = sqlite3_step(this->_control->stmt);
		if (this->_prevStep != SQLITE_ROW && this->_prevStep != SQLITE_DONE) {
			throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(sqlite3_db_handle(this->_control->stmt)));
		}
	}
	return *this;
}

bool operator==(const SQLiteIterator& i, const SQLiteViewSentinel& s) {
	return i._prevStep == SQLITE_DONE;
}

bool operator==(const SQLiteViewSentinel& s, const SQLiteIterator& i) {
	return i._prevStep == SQLITE_DONE;
}

SQLiteIterator::SQLiteIterator(SQLiteIterator&& x) noexcept {
	*this = std::move(x);
}

SQLiteIterator& SQLiteIterator::operator=(SQLiteIterator&& x) noexcept {
	this->_control = std::move(x._control);
	return *this;
}

SQLiteView::SQLiteView(std::shared_ptr<SQLiteStmtControl> control) : _control(control) {
	this->_control->keep(SQLiteStmtControl::ENABLE_SQLITE_VIEW);
}

SQLiteView::~SQLiteView() {
	this->_control->dispose(SQLiteStmtControl::ENABLE_SQLITE_VIEW);
}

SQLiteIterator SQLiteView::begin() const {
	if (this->_beginCalled) {
		throw std::logic_error("2回以上beginを呼び出すことは不正です");
	}

	// SQLの1行目の取得を試みる
	int prevStep = sqlite3_step(this->_control->stmt);
	if (prevStep != SQLITE_ROW && prevStep != SQLITE_DONE) {
		throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(sqlite3_db_handle(this->_control->stmt)));
	}
	this->_beginCalled = true;
	return SQLiteIterator(this->_control, prevStep);
}

SQLiteViewSentinel SQLiteView::end() const {
	return SQLiteViewSentinel();
}

SQLiteView::SQLiteView(SQLiteView&& x) noexcept {
	*this = std::move(x);
}

SQLiteView& SQLiteView::operator=(SQLiteView&& x) noexcept {
	this->_control = std::move(x._control);
	return *this;
}
