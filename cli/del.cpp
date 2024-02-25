#include "del.h"
#include "CommandLineOption.hpp"
#include "common.h"
#include "PasswordManagement.h"

void del(int argc, const char* argv[], const std::filesystem::path& db, std::ostream& os) {
    option::CommandLineOption clo;
    clo.add_options()
        .l(od_help.name, od_help.summary)
        .l(od_help_with_target.name, option::Value<std::string>().name("option"), od_help_with_target.summary);
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

    // DBとのコネクションを確立してデータの削除を行う
    auto conn = SQLite(db);
    auto pm = pwm::PasswordManagement(db, conn);
    pm.remove(data);

}
