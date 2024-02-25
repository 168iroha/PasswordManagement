#pragma once

#include <optional>
#include <filesystem>
#include <chrono>
#include <vector>
#include "SQLiteConnection.h"
#include "SQLiteView.h"

namespace pwm {

	namespace table {

		/// <summary>
		/// パスワード管理テーブルの情報の定義
		/// </summary>
		struct passwords {
			static constexpr std::u8string_view value = u8"passwords";

			struct c_service { static constexpr std::u8string_view value = u8"service"; static constexpr int index = 0; };
			struct c_name { static constexpr std::u8string_view value = u8"name"; static constexpr int index = 1; };
			struct c_user { static constexpr std::u8string_view value = u8"user"; static constexpr int index = 2; };
			struct c_password { static constexpr std::u8string_view value = u8"password"; static constexpr int index = 3; };
			struct c_encryption { static constexpr std::u8string_view value = u8"encryption"; static constexpr int index = 4; };
			struct c_memo { static constexpr std::u8string_view value = u8"memo"; static constexpr int index = 5; };
			struct c_registered_at { static constexpr std::u8string_view value = u8"registered_at"; static constexpr int index = 6; };
			struct c_update_at { static constexpr std::u8string_view value = u8"update_at"; static constexpr int index = 7; };
		};

		/// <summary>
		/// 暗号化方式の定義
		/// </summary>
		struct encryption_method {
			static constexpr std::u8string_view none = u8"None";
		};
	}

	/// <summary>
	/// パスワード情報の取得のために用いるパラメータ
	/// </summary>
	struct GetParam {
		/// <summary>
		/// サービス名
		/// </summary>
		std::optional<std::u8string> service = std::nullopt;

		/// <summary>
		/// ユーザ名
		/// </summary>
		std::optional<std::u8string> user = std::nullopt;

		/// <summary>
		/// 名称(これが指定されたときはあらゆる検索条件が無視される)
		/// </summary>
		std::optional<std::u8string> name = std::nullopt;

		/// <summary>
		/// パスワードの登録日時の始端
		/// </summary>
		std::optional<std::chrono::utc_seconds> begin_registered_at = std::nullopt;
		/// <summary>
		/// パスワードの登録日時の終端
		/// </summary>
		std::optional<std::chrono::utc_seconds> end_registered_at = std::nullopt;

		/// <summary>
		/// パスワードの更新日時の始端
		/// </summary>
		std::optional<std::chrono::utc_seconds> begin_update_at = std::nullopt;
		/// <summary>
		/// パスワードの更新日時の終端
		/// </summary>
		std::optional<std::chrono::utc_seconds> end_update_at = std::nullopt;
	};

	/// <summary>
	/// パスワード情報の挿入のために用いるパラメータ
	/// </summary>
	struct InsertParam {
		/// <summary>
		/// サービス名
		/// </summary>
		std::u8string service;

		/// <summary>
		/// ユーザ名
		/// </summary>
		std::u8string user;

		/// <summary>
		/// 名称
		/// </summary>
		std::optional<std::u8string> name = std::nullopt;

		/// <summary>
		/// パスワード
		/// </summary>
		std::vector<unsigned char> password;

		/// <summary>
		/// メモ
		/// </summary>
		std::optional<std::u8string> memo = std::nullopt;
	};

	/// <summary>
	/// パスワード情報の挿入のために用いるパラメータ
	/// </summary>
	struct UpdateParam {
		/// <summary>
		/// サービス名
		/// </summary>
		std::optional<std::u8string> service = std::nullopt;

		/// <summary>
		/// ユーザ名
		/// </summary>
		std::optional<std::u8string> user = std::nullopt;

		/// <summary>
		/// 名称
		/// </summary>
		std::optional<std::optional<std::u8string>> name = std::nullopt;

		/// <summary>
		/// パスワード
		/// </summary>
		std::optional<std::vector<unsigned char>> password = std::nullopt;

		/// <summary>
		/// メモ
		/// </summary>
		std::optional<std::optional<std::u8string>> memo = std::nullopt;
	};

	/// <summary>
	/// パスワード管理を行うクラス
	/// </summary>
	class PasswordManagement {
		/// <summary>
		/// データベースへのパス
		/// </summary>
		std::filesystem::path _dbpath;

		/// <summary>
		/// SQLiteに関する操作の起点となるオブジェクト
		/// </summary>
		SQLite& _conn;
	public:
		PasswordManagement() = delete;
		PasswordManagement(const std::filesystem::path& dbpath, SQLite& conn);

		/// <summary>
		/// パスワード情報を挿入する
		/// </summary>
		/// <param name="obj">挿入情報</param>
		void insert(const InsertParam& obj);

		/// <summary>
		/// パスワード情報を更新する
		/// </summary>
		/// <param name="obj">更新条件</param>
		/// <param name="content">更新内容</param>
		void update(const GetParam& obj, const UpdateParam& content);

		/// <summary>
		/// パスワード情報を取得する
		/// </summary>
		/// <param name="obj">取得条件</param>
		/// <param name="target">取得対象(passwordsのカラムに関連付けられたインデックス)</param>
		/// <returns>SQLの実行結果の取得のためのView</returns>
		[[nodiscard]] SQLiteView get(const GetParam& obj, const std::vector<int>& target_list);

		/// <summary>
		/// パスワード情報を削除する
		/// </summary>
		/// <param name="obj">削除条件</param>
		void remove(const GetParam& obj);
	};
}
