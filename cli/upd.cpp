#include "upd.h"
#include "CommandLineOption.hpp"
#include "common.h"
#include "PasswordManagement.h"

namespace {

    const OptionDetail od_service_to = {
        .name = "srv-to ",
        .summary = "パスワード情報における更新するサービス名",
        .detail = "パスワード情報における更新するサービス名"
    };
    const OptionDetail od_user_to = {
        .name = "user-to ",
        .summary = "パスワード情報における更新するユーザ名",
        .detail = "パスワード情報における更新するユーザ名"
    };

    const OptionDetail od_name_to = {
        .name = "name-to ",
        .summary = "パスワード管理において更新するパスワード情報を示す識別子",
        .detail = "パスワード管理において更新するパスワード情報を示す識別子"
    };

    const OptionDetail od_password_to = {
        .name = "pw-to ",
        .summary = "パスワード情報における更新するパスワード",
        .detail = "パスワード情報における更新するパスワード"
    };

    const OptionDetail od_memo_to = {
        .name = "memo-to ",
        .summary = "パスワード情報に対して更新する補足する事項",
        .detail = "パスワード情報に対して更新する補足する事項"
    };

}

void upd(int argc, const char* argv[], const std::filesystem::path& db, std::ostream& os) {
    option::CommandLineOption clo;
    clo.add_options()
        .l(od_help.name, od_help.summary)
        .l(od_help_with_target.name, option::Value<std::string>().name("option"), od_help_with_target.summary)
        .l(od_service_to.name, option::Value<std::string>().name("service"), od_service_to.summary)
        .l(od_user_to.name, option::Value<std::string>().name("user"), od_user_to.summary)
        .l(od_name_to.name, option::Value<std::string>().name("name"), od_name_to.summary)
        .l(od_password_to.name, option::Value<std::string>().name("password"), od_password_to.summary)
        .l(od_memo_to.name, option::Value<std::string>().name("memo"), od_memo_to.summary);
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
        else if (target == od_service_to.name) {
            detail = od_service_to.detail;
        }
        else if (target == od_user_to.name) {
            detail = od_user_to.detail;
        }
        else if (target == od_name_to.name) {
            detail = od_name_to.detail;
        }
        else if (target == od_password_to.name) {
            detail = od_password_to.detail;
        }
        else if (target == od_memo_to.name) {
            detail = od_memo_to.detail;
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

    // 更新対象を示すデータの構築
    pwm::UpdateParam updateData;
    if (auto temp = map.use(od_service_to.name); temp) {
        updateData.service = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
    }
    if (auto temp = map.use(od_user_to.name); temp) {
        updateData.user = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
    }
    if (auto temp = map.use(od_name_to.name); temp) {
        auto name = temp.as<std::string>();
        if (name.length() == 0) {
            updateData.name = std::optional<std::u8string>();
        }
        else {
            updateData.name = std::bit_cast<char8_t*>(name.c_str());
        }
    }
    if (auto temp = map.use(od_password_to.name); temp) {
        auto password = temp.as<std::string>();
        updateData.password = std::vector<unsigned char>(password.begin(), password.end());
    }
    if (auto temp = map.use(od_memo_to.name); temp) {
        auto memo = temp.as<std::string>();
        if (memo.length() == 0) {
            updateData.memo = std::optional<std::u8string>();
        }
        else {
            updateData.memo = std::bit_cast<char8_t*>(memo.c_str());
        }
    }

    // 検索条件を示すデータの構築
    pwm::GetParam getData = cond::getGetParam(map);

    // DBとのコネクションを確立してデータの更新を行う
    auto conn = SQLite(db);
    auto pm = pwm::PasswordManagement(db, conn);
    pm.update(getData, updateData);
}
