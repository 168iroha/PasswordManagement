#include <iostream>
#include <unordered_map>
#include <filesystem>
#include "CommandLineOption.hpp"
#include "get.h"
#include "ins.h"
#include "upd.h"
#include "del.h"
#include "common.h"
#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace {

    const OptionDetail od_target = {
        .name = "target",
        .summary = "結果の出力先",
        .detail = "<command>で指定したコマンドの出力先を指定する\n"
        "-oオプションを指定したときは自動的にfileと解釈される\n"
        "  stdout  標準出力へ出力\n"
        "  file    ファイルへ出力"
    };

    const OptionDetail od_output = {
        .name = "o",
        .summary = "結果の出力先のファイルパス",
        .detail = "<command>で指定したコマンドの出力先を指定する\n"
        "-oオプションを指定したときは自動的にfileと解釈される\n"
        "  stdout  標準出力へ出力\n"
        "  file    ファイルへ出力"
    };

    const OptionDetail od_command = {
        .name = "command",
        .summary = "実行するコマンド",
        .detail = "以下のいずれかを指定してコマンドを実行する\n"
        "  make    パスワードを生成する\n"
        "  get     パスワード情報を取得する\n"
        "  ins     パスワード情報を挿入する\n"
        "  upd     パスワード情報を更新する\n"
        "  del     パスワード情報を削除する"
    };

    /// <summary>
    /// コマンド名と変数および関数を紐づけるための構造体
    /// </summary>
    struct CommandDetail {
        /// <summary>
        /// コマンドの実行に関する関数
        /// </summary>
        void (*callback)(int, const char* [], const std::filesystem::path&, std::ostream&);
    };

    std::unordered_map<std::string, CommandDetail> cd_map = {
        { "get", {.callback = get }},
        { "ins", {.callback = ins }},
        { "upd", {.callback = upd }},
        { "del", {.callback = del }}
    };
}

int main(int argc, const char* argv[]) {
#if defined(_MSC_VER)
    // UTF-8でコンソールに出力するための設定
    SetConsoleOutputCP(CP_UTF8);
#endif
    option::CommandLineOption clo;
    clo.add_options()
        .l(od_help.name, od_help.summary)
        .l(od_help_with_target.name, option::Value<std::string>().name("option"), od_help_with_target.summary)
        .l(od_target.name, option::Value<std::string>("stdout").name("type"), od_target.summary)
        .o(od_output.name, option::Value<std::string>().name("out"), od_output.summary)
        // コマンドが入力されたらそそれ以降は別の解析器で解析する
        .u.pause()(option::Value<std::string>().name(od_command.name), od_command.summary);

    if (argc == 1) {
        // 引数が存在しないときは説明を表示
        std::cout << "Options:" << std::endl;
        std::cout << clo.description() << std::endl;
        return 0;
    }

    const option::OptionMap& map = clo.map();
    int suboffset = 0;
    try {
        // コマンドライン引数の解析の実行
        suboffset = clo.parse(argc - 1, &argv[1]);
    }
    catch (std::runtime_error& e) {
        // 解析エラーが生じたときにその内容を表示
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

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
        else if (target == od_target.name) {
            detail = od_target.detail;
        }
        else if (target == od_output.name) {
            detail = od_output.detail;
        }
        else if (target == od_command.name) {
            detail = od_command.detail;
        }
        else {
            std::cerr << target << " に該当する説明は存在しません" << std::endl;
            return 1;
        }
        std::cout << detail << std::endl;
        return 0;
    }
    else if (auto temp = map.luse(od_help.name); temp) {
        // コマンド一覧を表示
        std::cout << "Options:" << std::endl;
        std::cout << clo.description() << std::endl;
        return 0;
    }

    if (auto temp = map.unnamed_options(); temp) {
        // コマンドの実行
        auto command = temp.as<std::string>();
        std::filesystem::path dbname = std::filesystem::path(argv[0]).remove_filename() / u8"pwm.db";

        if (cd_map.contains(command)) {
            try {
                cd_map.at(command).callback(argc - 1 - suboffset, &argv[1 + suboffset], dbname, std::cout);
            }
            catch (std::exception& e) {
                std::cerr << "error: " << e.what() << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << command << " に該当するコマンドは存在しません" << std::endl;
            return 1;
        }
    }
    else {
        std::cerr << "実行するコマンドが指定されていません" << std::endl;
        return 1;
    }

    return 0;
}