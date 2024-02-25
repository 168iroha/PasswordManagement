#include "get.h"
#include "CommandLineOption.hpp"
#include "common.h"
#include "PasswordManagement.h"
#include <unordered_map>

namespace {

    const OptionDetail od_col = {
        .name = "col ",
        .summary = "取得する対象項目",
        .detail = "以下のような取得する対象項目を指定する\n"
        "  srv     サービス名\n"
        "  user    ユーザ名\n"
        "  name    名称\n"
        "  pw      パスワード\n"
        "  memo    メモ\n"
        "  reg     登録日時\n"
        "  upd     更新日時"
    };

    /// <summary>
    /// 表示可能なカラムの一覧についての列挙
    /// </summary>
    struct col_list {
        static constexpr std::u8string_view service = u8"srv";
        static constexpr std::u8string_view user = u8"user";
        static constexpr std::u8string_view name = u8"name";
        static constexpr std::u8string_view password = u8"pw";
        static constexpr std::u8string_view memo = u8"memo";
        static constexpr std::u8string_view registered_at = u8"reg";
        static constexpr std::u8string_view update_at = u8"upd";
    };
    const std::unordered_map<std::u8string_view, int> col_map = {
        { col_list::service, pwm::table::passwords::c_service::index },
        { col_list::name, pwm::table::passwords::c_name::index },
        { col_list::user, pwm::table::passwords::c_user::index },
        { col_list::password, pwm::table::passwords::c_password::index },
        { col_list::memo, pwm::table::passwords::c_memo::index },
        { col_list::registered_at, pwm::table::passwords::c_registered_at::index },
        { col_list::update_at, pwm::table::passwords::c_update_at::index }
    };
}

void get(int argc, const char* argv[], const std::filesystem::path& db, std::ostream& os) {
    option::CommandLineOption clo;
    clo.add_options()
        .l(od_help.name, od_help.summary)
        .l(od_help_with_target.name, option::Value<std::string>().name("option"), od_help_with_target.summary)
        .l(od_col.name, option::Value<std::string>({
            std::bit_cast<char*>(col_list::service.data()),
            std::bit_cast<char*>(col_list::user.data()),
            std::bit_cast<char*>(col_list::password.data())
        }).unlimited().constraint([](const std::string& x) { return col_map.contains(std::bit_cast<char8_t*>(x.data())); }).name("col"), od_col.summary);
    cond::addCond(clo.add_options());

    if (argc == 0) {
        // 引数が存在しないときは説明を表示
        std::cout << "Options:" << std::endl;
        std::cout << clo.description() << std::endl;
        return;
    }

    const option::OptionMap& map = clo.map();
    // コマンドライン引数の解析の実行
    clo.parse(argc, argv, false);

    if (auto temp = map.luse(od_help_with_target.name); temp) {
        // コマンドライン引数に対する説明の表示
        auto target = temp.as<std::string>();
        std::string detail;
        if (target == od_help.name) {
            detail = od_help.detail;
        }
        else if (target == od_help_with_target.name) {
            detail = od_help_with_target.detail;
        }
        else if (target == od_col.name) {
            detail = od_col.detail;
        }
        else if (cond::getDetail(target, detail));
        else {
            std::cerr << target << " に該当する説明は存在しません" << std::endl;
            return;
        }
        std::cout << detail << std::endl;
        return;
    }
    else if (auto temp = map.luse(od_help.name); temp) {
        // コマンド一覧を表示
        std::cout << "Options:" << std::endl;
        std::cout << clo.description() << std::endl;
        return;
    }

    // 入力値の評価
    map.validate();

    // 検索条件を示すデータの構築
    pwm::GetParam data = cond::getGetParam(map);

    // DBとのコネクションを確立してデータの取得を行う
    auto conn = SQLite(db);
    auto pm = pwm::PasswordManagement(db, conn);
    using pws = pwm::table::passwords;
    using namespace std::ranges;
    // 入力として与えられる文字列からインデックスへの変換
    auto cols = map.use(od_col.name).as<std::vector<std::string>>() |
        views::transform([](const std::string& x) {
            return col_map.at(std::bit_cast<char8_t*>(x.data()));
        }) | to<std::vector<int>>();
    for (auto e : pm.get(data, cols)) {
        int cnt = 0;
        for (int col : cols) {
            if (cnt > 0) {
                os << ",";
            }
            // カラムごとに決められた型で出力する
            switch (col) {
            case pws::c_service::index:
            case pws::c_name::index:
            case pws::c_user::index:
            case pws::c_encryption::index:
            case pws::c_memo::index:
                os << std::bit_cast<char*>(e.get<SQLiteData::string_type>(cnt).value_or(u8"null").data());
                break;
            case pws::c_registered_at::index:
            case pws::c_update_at::index:
            {
                // ロケールで補正した時刻を出力する
                std::stringstream ss(std::bit_cast<char*>(e.get<SQLiteData::string_type>(cnt).value().data()));
                std::chrono::utc_seconds t;
                const std::chrono::time_zone* time_zone = std::chrono::current_zone();
                std::chrono::from_stream(ss, "%Y-%m-%d-%H-%M-%S", t);
                os << std::format("{:%Y-%m-%d %H:%M:%S}", t + time_zone->get_info(std::chrono::utc_clock::to_sys(t)).offset);
                break;
            }
            case pws::c_password::index:
            {
                auto optional = e.get<SQLiteData::blob_type>(cnt);
                SQLiteData::blob_type blob = optional.value();
                // 現状はBLOBも文字列化して出力する
                os << std::string(blob.begin(), blob.end());
                break;
            }
            }
            ++cnt;
        }
        os << std::endl;
    }
}
