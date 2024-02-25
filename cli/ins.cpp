#include "ins.h"
#include "CommandLineOption.hpp"
#include "common.h"
#include "PasswordManagement.h"

namespace {

    const OptionDetail od_service = {
        .name = "srv ",
        .summary = "パスワード情報におけるサービス名",
        .detail = "パスワード情報におけるサービス名であり、例えば以下を指定する\n"
        "  パスワードの保存の対象のサイトのURL\n"
        "  パスワード認証が必要なアカウントの管理元の名称"
    };
    const OptionDetail od_user = {
        .name = "user ",
        .summary = "パスワード情報におけるユーザ名",
        .detail = "パスワード情報におけるユーザ名であり、例えば以下を指定する\n"
        "  利用者を紐づけるメールアドレスなどの文字列\n"
        "  サービス名とのペアで利用者を特定できる情報"
    };

    const OptionDetail od_name = {
        .name = "name ",
        .summary = "パスワード管理においてパスワード情報を示す識別子",
        .detail = "パスワード管理においてパスワード情報を示す識別子"
    };

    const OptionDetail od_password = {
        .name = "pw ",
        .summary = "パスワード情報におけるパスワード",
        .detail = "パスワード情報におけるパスワード"
    };

    const OptionDetail od_memo = {
        .name = "memo ",
        .summary = "パスワード情報に対して補足する事項",
        .detail = "パスワード情報に対して補足する事項"
    };
}

void ins(int argc, const char* argv[], const std::filesystem::path& db, std::ostream& os) {
    option::CommandLineOption clo;
    clo.add_options()
        .l(od_help.name, od_help.summary)
        .l(od_help_with_target.name, option::Value<std::string>().name("option"), od_help_with_target.summary)
        .l(od_service.name, option::Value<std::string>().name("service").required(), od_service.summary)
        .l(od_user.name, option::Value<std::string>().name("user").required(), od_user.summary)
        .l(od_name.name, option::Value<std::string>().name("name"), od_name.summary)
        .l(od_password.name, option::Value<std::string>().name("password"), od_password.summary)
        .l(od_memo.name, option::Value<std::string>().name("memo"), od_memo.summary);

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
        else if (target == od_service.name) {
            detail = od_service.detail;
        }
        else if (target == od_user.name) {
            detail = od_user.detail;
        }
        else if (target == od_name.name) {
            detail = od_name.detail;
        }
        else if (target == od_password.name) {
            detail = od_password.detail;
        }
        else if (target == od_memo.name) {
            detail = od_memo.detail;
        }
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

    // 挿入を行うデータの構築
    pwm::InsertParam data = {
        .service = std::bit_cast<char8_t*>(map.use(od_service.name).as<std::string>().c_str()),
        .user = std::bit_cast<char8_t*>(map.use(od_user.name).as<std::string>().c_str())
    };
    if (auto temp = map.use(od_name.name); temp) {
        data.name = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
    }
    if (auto temp = map.use(od_memo.name); temp) {
        data.memo = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
    }
    auto password = map.use(od_password.name).as<std::string>();
    data.password = std::vector<unsigned char>(password.begin(), password.end());

    // DBとのコネクションを確立してデータの更新を行う
    auto conn = SQLite(db);
    auto pm = pwm::PasswordManagement(db, conn);
    pm.insert(data);
}
