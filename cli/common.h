#pragma once

#include "CommandLineOption.hpp"
#include "PasswordManagement.h"

/// <summary>
/// オプション名と変数名を紐づけるための構造体
/// </summary>
struct OptionDetail {
    /// <summary>
    /// オプション名
    /// </summary>
    std::string name;
    /// <summary>
    /// オプションに関する概要の説明
    /// </summary>
    std::string summary;
    /// <summary>
    /// オプションに関する詳細の説明
    /// </summary>
    std::string detail;
};

/// <summary>
/// ヘルプに関するオプション
/// </summary>
extern const OptionDetail od_help;

/// <summary>
/// 対象を指定したヘルプオプション
/// </summary>
extern const OptionDetail od_help_with_target;

/// <summary>
/// 検索条件に関する名前空間
/// </summary>
namespace cond {

    /// <summary>
    /// オプションの追加
    /// </summary>
    /// <param name="x"></param>
    /// <returns>オプションの追加の記述のためのET</returns>
    option::AddOptions& addCond(option::AddOptions x);

    /// <summary>
    /// オプションの詳細の取得
    /// </summary>
    /// <param name="target">オプション名</param>
    /// <param name="x">オプションの詳細を格納する変数</param>
    /// <returns>オプションの詳細が取得できた場合にtrue</returns>
    bool getDetail(const std::string& target, std::string& x);

    /// <summary>
    /// mapから抽出条件を示すオブジェクトを生成
    /// </summary>
    /// <param name="map">コマンドライン引数の解析結果</param>
    /// <returns>抽出条件を示すオブジェクト</returns>
    pwm::GetParam getGetParam(const option::OptionMap& map);
}
