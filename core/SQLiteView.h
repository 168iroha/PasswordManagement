#pragma once

#include "SQLiteStmt.h"
#include <span>
#include <string_view>
#include <ranges>

/// <summary>
/// データとして得る型(今回は利用するやつだけ定義する)
/// </summary>
template <class T>
concept data_value = std::disjunction_v<
    std::is_same<T, std::u8string_view>,
    std::is_same<T, std::span<unsigned char>>
>;

/// <summary>
/// SQLiteでSQLを実行した結果のデータ型
/// </summary>
class SQLiteData {
    /// <summary>
	/// 実行するSQLについてのステートメント
	/// </summary>
	sqlite3_stmt* _stmt = nullptr;
public:
	SQLiteData() = delete;
	SQLiteData(sqlite3_stmt& stmt);
	~SQLiteData() {}
 
	using string_type = std::u8string_view;
	using blob_type = std::span<unsigned char>;

	template <data_value T>
    [[nodiscard]] std::optional<T> get(int col);
};

/// <summary>
/// SQLiteViewのための番兵
/// </summary>
struct SQLiteViewSentinel {};

/// <summary>
/// SQLiteViewのためのイテレータ
/// </summary>
class SQLiteIterator {
    /// <summary>
    /// sqlite3_stmtの制御のための変数
    /// </summary>
    std::shared_ptr<SQLiteStmtControl> _control;
    /// <summary>
    /// 前回のsqlite3_stepの評価結果
    /// </summary>
    int _prevStep = SQLITE_DONE;

    friend SQLiteView;
    explicit SQLiteIterator(std::shared_ptr<SQLiteStmtControl> control, int prevStep);

public:
    using difference_type = int;
    using value_type = SQLiteData;
    using iterator_concept = std::input_iterator_tag;

    SQLiteIterator() = delete;
    ~SQLiteIterator();

    [[nodiscard]] SQLiteData operator*() const;
    SQLiteIterator& operator++();
    void operator++(int) { ++*this; }

    friend bool operator==(const SQLiteIterator& i, const SQLiteViewSentinel& s);
    friend bool operator==(const SQLiteViewSentinel& s, const SQLiteIterator& i);

    SQLiteIterator(SQLiteIterator&& x) noexcept;
    SQLiteIterator& operator=(SQLiteIterator&& x) noexcept;

    // コピーによる構築を禁止する
    SQLiteIterator(const SQLiteIterator&) = delete;
    SQLiteIterator& operator=(const SQLiteIterator&) = delete;
};

/// <summary>
/// SQLiteでSQL文を実行結果をイテレートするためのView
/// </summary>
class SQLiteView : public std::ranges::view_interface<SQLiteView> {
    /// <summary>
    /// sqlite3_stmtの制御のための変数
    /// </summary>
    std::shared_ptr<SQLiteStmtControl> _control;
    /// <summary>
    /// beginが既に呼び出されたことがあるかを示すフラグ
    /// </summary>
    mutable bool _beginCalled = false;

    friend SQLiteStmt;
    SQLiteView(std::shared_ptr<SQLiteStmtControl> control);
public:
    SQLiteView() = delete;
    ~SQLiteView();

    [[nodiscard]] SQLiteIterator begin() const;
    [[nodiscard]] SQLiteViewSentinel end() const;

    SQLiteView(SQLiteView&& x) noexcept;
    SQLiteView& operator=(SQLiteView&& x) noexcept;

    // コピーによる構築を禁止する
    SQLiteView(const SQLiteView&) = delete;
    SQLiteView& operator=(const SQLiteView&) = delete;
};
