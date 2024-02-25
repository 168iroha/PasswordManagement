#include "PasswordManagement.h"
#include <bit>
#include <iostream>
#include <sstream>
#include <ranges>

namespace pwm {
    namespace {
        /// <summary>
        /// passwordsに関するエイリアス
        /// </summary>
        using pws = table::passwords;

        /// <summary>
        /// パスワード管理で利用するテーブルの宣言
        /// </summary>
        static const std::u8string sql_cretate_table = std::bit_cast<const char8_t*>(std::format(R"(
            CREATE TABLE IF NOT EXISTS {0} (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                {1} TEXT NOT NULL,
                {2} TEXT NOT NULL,
                {3} TEXT UNIQUE,
                {4} BLOB NOT NULL,
                {5} TEXT NOT NULL,
                {6} TEXT,
                {7} TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                {8} TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
            );
            CREATE UNIQUE INDEX IF NOT EXISTS idx_{0}_00 ON {0}({1}, {2});
            CREATE INDEX IF NOT EXISTS idx_{0}_01 ON {0}({1});
            CREATE INDEX IF NOT EXISTS idx_{0}_02 ON {0}({3});
            CREATE INDEX IF NOT EXISTS idx_{0}_03 ON {0}({7});
            CREATE INDEX IF NOT EXISTS idx_{0}_04 ON {0}({8});
        )",
            // テーブル名の埋め込み
            std::bit_cast<const char*>(pws::value.data()),
            // サービス名の埋め込み
            std::bit_cast<const char*>(pws::c_service::value.data()),
            // ユーザ名の埋め込み
            std::bit_cast<const char*>(pws::c_user::value.data()),
            // 名称の埋め込み
            std::bit_cast<const char*>(pws::c_name::value.data()),
            // パスワード名の埋め込み
            std::bit_cast<const char*>(pws::c_password::value.data()),
            // 暗号化の名称の埋め込み
            std::bit_cast<const char*>(pws::c_encryption::value.data()),
            // メモ名の埋め込み
            std::bit_cast<const char*>(pws::c_memo::value.data()),
            // パスワードの登録日時名の埋め込み
            std::bit_cast<const char*>(pws::c_registered_at::value.data()),
            // パスワードの更新日時名の埋め込み
            std::bit_cast<const char*>(pws::c_update_at::value.data())
        ).data());

        /// <summary>
        /// パスワードを登録するSQLの宣言
        /// </summary>
        std::u8string sql_insert = std::bit_cast<const char8_t*>(std::format(R"(
            INSERT INTO {0} ({1}, {2}, {3}, {4}, {5}, {6}) VALUES (?, ?, ?, ?, ?, ?);
        )",
            // テーブル名の埋め込み
            std::bit_cast<const char*>(pws::value.data()),
            // サービス名の埋め込み
            std::bit_cast<const char*>(pws::c_service::value.data()),
            // ユーザ名の埋め込み
            std::bit_cast<const char*>(pws::c_user::value.data()),
            // 名称の埋め込み
            std::bit_cast<const char*>(pws::c_name::value.data()),
            // パスワード名の埋め込み
            std::bit_cast<const char*>(pws::c_password::value.data()),
            // 暗号化の名称の埋め込み
            std::bit_cast<const char*>(pws::c_encryption::value.data()),
            // メモ名の埋め込み
            std::bit_cast<const char*>(pws::c_memo::value.data())
        ).data());

        /// <summary>
        /// 検索条件におけるWhere句を示す文字列を構築
        /// </summary>
        /// <param name="obj">検索条件</param>
        /// <returns></returns>
        std::u8string getWhereStr(const GetParam& obj) {
            std::u8string where_str;
            if (obj.name) {
                // nameが指定されたときは他の条件をすべて無視する
                where_str += pws::c_name::value;
                where_str += u8"=?";
            }
            else {
                if (obj.service) {
                    where_str += pws::c_service::value;
                    where_str += u8"=?";
                }
                if (obj.user) {
                    where_str += pws::c_user::value;
                    where_str += u8"=?";
                }
                if (obj.begin_registered_at) {
                    if (obj.end_registered_at) {
                        where_str += pws::c_registered_at::value;
                        where_str += u8" BETWEEN ? AND ?";
                    }
                    else {
                        where_str += pws::c_registered_at::value;
                        where_str += u8">=?";
                    }
                }
                else if (obj.end_registered_at) {
                    where_str += pws::c_registered_at::value;
                    where_str += u8"<=?";
                }
                if (obj.begin_update_at) {
                    if (obj.end_update_at) {
                        where_str += pws::c_update_at::value;
                        where_str += u8" BETWEEN ? AND ?";
                    }
                    else {
                        where_str += pws::c_update_at::value;
                        where_str += u8">=?";
                    }
                }
                else if (obj.end_update_at) {
                    where_str += pws::c_update_at::value;
                    where_str += u8"<=?";
                }
            }
            return where_str.length() == 0 ? u8"" : (u8"WHERE " + where_str);
        }

        /// <summary>
        /// Where句に関するバインド変数を設定
        /// </summary>
        /// <param name="stmt"></param>
        /// <param name="obj"></param>
        /// <param name="offset"></param>
        /// <returns></returns>
        int bindWhere(SQLiteStmt& stmt, const GetParam& obj, int offset) {
            // コードの構造は抽出条件のgetWhereStrと同じ

            if (obj.name) { stmt.bind(offset++, obj.name); }
            else {
                if (obj.service) { stmt.bind(offset++, obj.service); }
                if (obj.user) { stmt.bind(offset++, obj.user); }
                if (obj.begin_registered_at) {
                    if (obj.end_registered_at) {
                        stmt.bind(offset++, obj.begin_registered_at);
                        stmt.bind(offset++, obj.end_registered_at);
                    }
                    else { stmt.bind(offset++, obj.begin_registered_at); }
                }
                else if (obj.end_registered_at) { stmt.bind(offset++, obj.end_registered_at); }
                if (obj.begin_update_at) {
                    if (obj.end_update_at) {
                        stmt.bind(offset++, obj.begin_update_at);
                        stmt.bind(offset++, obj.end_update_at);
                    }
                    else { stmt.bind(offset++, obj.begin_update_at); }
                }
                else if (obj.end_update_at) { stmt.bind(offset++, obj.end_registered_at); }
            }
            return offset;
        }
    }

    PasswordManagement::PasswordManagement(const std::filesystem::path& dbpath, SQLite& conn): _dbpath(dbpath), _conn(conn) {
        if (this->_conn) {
            // テーブルを構築
            this->_conn.exec(sql_cretate_table);
        }
        else {
            throw std::runtime_error("DBとのコネクションが確立されていません");
        }
    }

    void PasswordManagement::insert(const InsertParam& obj) {
        if (this->_conn) {
            auto stmt = this->_conn.prepare(sql_insert);
            // バインド変数へ設定
            stmt.bind(1, obj.service);
            stmt.bind(2, obj.user);
            stmt.bind(3, obj.name);
            stmt.bind(4, obj.password);
            stmt.bind(5, pwm::table::encryption_method::none);
            stmt.bind(6, obj.memo);
            // パスワード情報を挿入
            for (const auto& x : stmt.exec()) {}
        }
        else {
            throw std::runtime_error("DBとのコネクションが確立されていません");
        }
    }
    void PasswordManagement::update(const GetParam& obj, const UpdateParam& content) {
        if (this->_conn) {
            std::vector<std::u8string_view> update_list = {};
            if (content.service) {
                update_list.push_back(pws::c_service::value);
            }
            if (content.user) {
                update_list.push_back(pws::c_user::value);
            }
            if (content.name) {
                update_list.push_back(pws::c_name::value);
            }
            if (content.password) {
                update_list.push_back(pws::c_password::value);
            }
            if (content.memo) {
                update_list.push_back(pws::c_memo::value);
            }
            using namespace std::ranges;
            // 更新に関するクエリ部分の構築
            auto cols = update_list |
                views::transform([](std::u8string_view x) {
                    return u8"," + std::u8string(x.begin(), x.end()) + u8"=?";
                }) | views::join | to<std::u8string>();

            // 抽出条件のSQLの構築
            std::u8string where_str = getWhereStr(obj);

            std::u8string sql_update = std::bit_cast<const char8_t*>(std::format(R"(
                UPDATE {0} SET {1}=CURRENT_TIMESTAMP{2} {3};
            )",
                // テーブル名の埋め込み
                std::bit_cast<const char*>(pws::value.data()),
                // パスワードの更新日時名の埋め込み
                std::bit_cast<const char*>(pws::c_update_at::value.data()),
                // 更新に関するクエリ部分の埋め込み
                std::bit_cast<const char*>(cols.data()),
                // WHERE句の埋め込み
                std::bit_cast<const char*>(where_str.data())
            ).data());

            // バインド変数の設定
            auto stmt = this->_conn.prepare(sql_update);
            int offset = 1;
            if (content.service) {
                stmt.bind(offset++, content.service.value());
            }
            if (content.user) {
                stmt.bind(offset++, content.user.value());
            }
            if (content.name) {
                stmt.bind(offset++, content.name.value());
            }
            if (content.password) {
                stmt.bind(offset++, content.password.value());
            }
            if (content.memo) {
                stmt.bind(offset++, content.memo.value());
            }
            if (where_str.length() != 0) {
                bindWhere(stmt, obj, offset);
            }

            // パスワード情報を更新
            for (const auto& x : stmt.exec()) {}
        }
        else {
            throw std::runtime_error("DBとのコネクションが確立されていません");
        }
    }
    SQLiteView PasswordManagement::get(const GetParam& obj, const std::vector<int>& target_list) {
        using namespace std::ranges;
        if (this->_conn) {
            // 取得対象のカラムに関するSQLの構築
            std::vector<std::u8string_view> col_list;
            for (const auto& i : target_list) {
                switch (i) {
                case pws::c_service::index:
                    col_list.emplace_back(pws::c_service::value);
                    break;
                case pws::c_name::index:
                    col_list.emplace_back(pws::c_name::value);
                    break;
                case pws::c_user::index:
                    col_list.emplace_back(pws::c_user::value);
                    break;
                case pws::c_password::index:
                    col_list.emplace_back(pws::c_password::value);
                    break;
                case pws::c_encryption::index:
                    col_list.emplace_back(pws::c_encryption::value);
                    break;
                case pws::c_memo::index:
                    col_list.emplace_back(pws::c_memo::value);
                    break;
                case pws::c_registered_at::index:
                    col_list.emplace_back(pws::c_registered_at::value);
                    break;
                case pws::c_update_at::index:
                    col_list.emplace_back(pws::c_update_at::value);
                    break;
                }
            }
            if (col_list.size() == 0) {
                throw std::invalid_argument("取得対象として指定された列が空です");
            }
            std::u8string col_list_str = col_list | views::join_with(u8',') | to<std::u8string>();

            // 抽出条件のSQLの構築
            std::u8string where_str = getWhereStr(obj);

            std::u8string sql_select = std::bit_cast<const char8_t*>(std::format(R"(
                SELECT {0} FROM {1} {2} ORDER BY id;
            )",
                // カラム名の埋め込み
                std::bit_cast<const char*>(col_list_str.data()),
                // テーブル名の埋め込み
                std::bit_cast<const char*>(pws::value.data()),
                // WHERE句の埋め込み
                std::bit_cast<const char*>(where_str.data())
            ).data());

            // バインド変数の設定
            auto stmt = this->_conn.prepare(sql_select);
            if (where_str.length() != 0) {
                bindWhere(stmt, obj, 1);
            }

            return stmt.exec();
        }
        else {
            throw std::runtime_error("DBとのコネクションが確立されていません");
        }
    }
    void PasswordManagement::remove(const GetParam& obj) {
        // 抽出条件のSQLの構築
        std::u8string where_str = getWhereStr(obj);

        std::u8string sql_select = std::bit_cast<const char8_t*>(std::format(R"(
                DELETE  FROM {0} {1};
            )",
            // テーブル名の埋め込み
            std::bit_cast<const char*>(pws::value.data()),
            // WHERE句の埋め込み
            std::bit_cast<const char*>(where_str.data())
        ).data());

        // バインド変数の設定
        auto stmt = this->_conn.prepare(sql_select);
        if (where_str.length() != 0) {
            bindWhere(stmt, obj, 1);
        }

        // パスワード情報を削除
        for (const auto& x : stmt.exec()) {}
    }
}
