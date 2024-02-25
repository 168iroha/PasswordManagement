#pragma once

#include <iostream>

/// <summary>
/// insコマンドの実行
/// </summary>
/// <param name="argc">コマンドライン引数の個数</param>
/// <param name="argv">コマンドライン引数の配列</param>
/// <param name="db">DBデータへのパス</param>
/// <param name="os">出力ストリーム</param>
void ins(int argc, const char* argv[], const std::filesystem::path& db, std::ostream& os);
