#pragma once

#include "sqlite3.h"
#include <type_traits>
#include <string>
#include <vector>
#include <optional>
#include <chrono>

struct SQLiteConnection;

/// <summary>
/// バインドを行う型(今回は利用するやつだけ定義する)
/// </summary>
template <class T>
concept bind_value = std::disjunction_v<
	std::is_same<T, std::u8string_view>,
	std::is_same<T, std::u8string>,
	std::is_same<T, std::chrono::utc_seconds>,
	std::is_same<T, nullptr_t>,
	std::is_same<T, std::vector<unsigned char>>
>;

/// <summary>
/// SQLiteStmtに関するインスタンスを制御するクラス
/// </summary>
struct SQLiteStmtControl {
	/// <summary>
	/// SQLiteとのコネクションのハンドラ
	/// </summary>
	std::shared_ptr<SQLiteConnection> conn;
	/// <summary>
	/// 実行するSQLについてのステートメント
	/// </summary>
	sqlite3_stmt* stmt = nullptr;
	/// <summary>
	/// sqlite3_stmtの制御のための変数
	/// </summary>
	std::size_t control = 0;

	/// <summary>
	/// sqlite3_stmtを保持する
	/// </summary>
	void keep(std::size_t mask) noexcept;

	/// <summary>
	/// sqlite3_stmtを破棄する
	/// </summary>
	void dispose(std::size_t mask) noexcept;

	SQLiteStmtControl(std::shared_ptr<SQLiteConnection> conn, sqlite3_stmt& stmt, std::size_t control);
	~SQLiteStmtControl();

	SQLiteStmtControl(SQLiteStmtControl&& x) noexcept;
	SQLiteStmtControl& operator=(SQLiteStmtControl&&) noexcept;

	/// <summary>
	/// SQLiteStmtが有効であることを示すビットフラグ
	/// </summary>
	static constexpr std::size_t ENABLE_SQLITE_STMT = 0b1;
	/// <summary>
	/// SQLiteViewが有効であることを示すビットフラグ
	/// </summary>
	static constexpr std::size_t ENABLE_SQLITE_VIEW = 0b10;
	/// <summary>
	/// SQLiteIteratorが有効であることを示すビットフラグ
	/// </summary>
	static constexpr std::size_t ENABLE_SQLITE_ITERATOR = 0b100;
};

class SQLiteView;

/// <summary>
/// SQLiteでSQL文を実行するためのクラス
/// </summary>
class SQLiteStmt {
	/// <summary>
	/// sqlite3_stmtの制御のための変数
	/// </summary>
	std::shared_ptr<SQLiteStmtControl> _control;

public:
	SQLiteStmt() = delete;
	SQLiteStmt(std::shared_ptr<SQLiteStmtControl> control);
	~SQLiteStmt();

	void bind(int index, std::u8string_view data);
	void bind(int index, const std::u8string& data);
	void bind(int index, const std::chrono::utc_seconds& data);
	void bind(int index, nullptr_t);
	void bind(int index, const std::vector<unsigned char>& data);
	template <bind_value T>
	void bind(int index, const std::optional<T>& data) {
		if (data) {
			this->bind(index, data.value());
		}
		else {
			this->bind(index, nullptr);
		}
	}

	/// <summary>
	/// SQLの実行結果の取得のためのViewを生成する
	/// </summary>
	/// <returns>SQLの実行結果の取得のためのView</returns>
	[[nodiscard]] SQLiteView exec();

	SQLiteStmt(SQLiteStmt&& x) noexcept;
	SQLiteStmt& operator=(SQLiteStmt&& x) noexcept;

	// コピーによる構築を禁止する
	SQLiteStmt(const SQLiteStmt&) = delete;
	SQLiteStmt& operator=(const SQLiteStmt&) = delete;
};
