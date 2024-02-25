#pragma once

#include "sqlite3.h"
#include <filesystem>
#include <variant>

class SQLiteStmt;

/// <summary>
/// SQLiteのコネクションを管理するクラス
/// </summary>
struct SQLiteConnection {
	/// <summary>
	/// SQLiteとのコネクションのハンドラ
	/// </summary>
	sqlite3* conn = nullptr;

	~SQLiteConnection();

	/// <summary>
	/// SQLiteとのコネクションを確立する
	/// </summary>
	/// <param name="path">データベースへのパス</param>
	void connect(const std::filesystem::path& path);

	/// <summary>
	/// SQLiteとのコネクションを切断する
	/// </summary>
	void disconnect();
};

/// <summary>
/// SQLiteに関する操作の起点となるクラス
/// </summary>
class SQLite {
	/// <summary>
	/// SQLiteとのコネクションのハンドラ
	/// </summary>
	std::shared_ptr<SQLiteConnection> _conn;

public:
	SQLite(const std::filesystem::path& path);

	/// <summary>
	/// trueならSQLiteとのコネクションが存在する
	/// </summary>
	operator bool() const { return this->_conn->conn != nullptr; }

	/// <summary>
	/// SQLを実行する
	/// </summary>
	/// <param name="sql">実行するSQL</param>
	void exec(const std::u8string& sql);

	/// <summary>
	/// プリペアドステートメントを作成する
	/// </summary>
	/// <param name="sql">実行するSQL</param>
	[[nodiscard]] SQLiteStmt prepare(const std::u8string& sql);
};
