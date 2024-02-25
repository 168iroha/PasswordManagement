#include "common.h"
#include <bit>

const OptionDetail od_help = {
    .name = "help",
    .summary = "コマンドラインオプションの表示",
    .detail = "コマンドラインオプションの表示"
};

const OptionDetail od_help_with_target = {
    .name = "help=",
    .summary = "コマンドラインオプションについてのヘルプ",
    .detail = "コマンドラインオプションについてのヘルプ"
};

namespace cond {

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

    const OptionDetail od_registered_at = {
        .name = "reg ",
        .summary = "パスワード情報の登録日時",
        .detail = "パスワード情報の登録日時"
    };

    const OptionDetail od_update_at = {
        .name = "upd ",
        .summary = "パスワード情報の更新日時",
        .detail = "パスワード情報の更新日時"
    };

    option::AddOptions& addCond(option::AddOptions x) {
        return x.l(od_service.name, option::Value<std::string>().name("service"), od_service.summary)
            .l(od_user.name, option::Value<std::string>().name("user"), od_user.summary)
            .l(od_name.name, option::Value<std::string>().name("name"), od_name.summary)
            .l(od_password.name, option::Value<std::string>().name("password"), od_password.summary)
            .l(od_registered_at.name, option::Value<std::string>().limit(2).name("registered_at"), od_registered_at.summary)
            .l(od_update_at.name, option::Value<std::string>().limit(2).name("update_at"), od_update_at.summary);
    }

    bool getDetail(const std::string& target, std::string& x) {
        // この分岐はそのうちどうにかする
        const std::string* p = nullptr;
        if (target == od_service.name) {
            p = std::addressof(od_service.detail);
        }
        else if (target == od_user.name) {
            p = std::addressof(od_user.detail);
        }
        else if (target == od_name.name) {
            p = std::addressof(od_name.detail);
        }
        else if (target == od_registered_at.name) {
            p = std::addressof(od_registered_at.detail);
        }
        else if (target == od_update_at.name) {
            p = std::addressof(od_update_at.detail);
        }
        if (p != nullptr) {
            x = *p;
            return true;
        }
        return false;
    }

    namespace {

        /// <summary>
        /// 文字列で表現される時刻をutc_secondsに変換
        /// </summary>
        /// <param name="time">文字列表現の時刻</param>
        /// <param name="round_up">時刻の切り上げを行うか</param>
        /// <returns></returns>
        inline std::chrono::utc_seconds to_utc_seconds(const std::string& time, bool round_up) {
            if (time.length() == 0) {
                throw std::invalid_argument("空の時刻を指定することはできません");
            }
            // 時刻補間に用いる文字列のリスト
            constexpr const char* interpolation[] = {
                "-01", "-01", "-00", "-00", "-00"
            };
            // 区切り文字を「-」に統一
            std::string time2 = time;
            std::ranges::replace(time2, ' ', '-');
            std::ranges::replace(time2, ':', '-');
            std::ranges::replace(time2, '/', '-');
            // 時刻の入力を補完
            std::size_t cnt = std::ranges::count(time2, '-');
            std::size_t temp = cnt;
            while (temp++ < 5) {
                time2 += interpolation[temp - 1];
            }
            // 文字列をsys_secondsに変換
            std::stringstream ss(time2);
            std::chrono::sys_seconds t;
            std::chrono::from_stream(ss, "%Y-%m-%d-%H-%M-%S", t);
            if (!ss) {
                throw std::runtime_error(std::format("異常な時刻[{0}]が指定されました", time));
            }

            if (round_up && cnt < 5) {
                // 未入力分の時刻補間部分を切り上げる
                switch (cnt) {
                case 0:
                    // %Y
                    t += std::chrono::years(1);
                    break;
                case 1:
                    // %Y-%m
                    t += std::chrono::months(1);
                    break;
                case 2:
                    // %Y-%m-%d
                    t += std::chrono::days(1);
                    break;
                case 3:
                    // %Y-%m-%d-%H
                    t += std::chrono::hours(1);
                    break;
                case 4:
                    // %Y-%m-%d-%H-%M
                    t += std::chrono::minutes(1);
                    break;
                }
                t -= std::chrono::seconds(1);
            }

            // タイムゾーンで補正した結果を返す
            const std::chrono::time_zone* time_zone = std::chrono::current_zone();
            return std::chrono::utc_clock::from_sys(t - time_zone->get_info(t).offset);
        }

        /// <summary>
        /// 検索条件に対して日時情報を設定する
        /// </summary>
        /// <param name="begin_at">日時の始端</param>
        /// <param name="end_at">日時の終端</param>
        /// <param name="x">空でない日時を示す文字列の配列</param>
        inline void setup_datetime(std::optional<std::chrono::utc_seconds>& begin_at, std::optional<std::chrono::utc_seconds>& end_at, const std::vector<std::string>& x) {
            if (x.size() == 1) {
                if (x[0].length() == 0) {
                    // 任意の日時を設定
                }
                else {
                    // x[0]で表現される範囲の日時を設定
                    begin_at = to_utc_seconds(x[0], false);
                    end_at = to_utc_seconds(x[0], true);
                }
            }
            else {
                if (x[0].length() == 0) {
                    if (x[1].length() == 0) {
                        // 任意の日時を設定
                    }
                    else {
                        // x[1]以前の日時を設定
                        end_at = to_utc_seconds(x[1], true);
                    }
                }
                else if (x[1].length() == 0) {
                    // x[0]以降の日時を設定
                    begin_at = to_utc_seconds(x[0], false);
                }
                else {
                    // x[0]とx[1]の区間の日時を設定
                    begin_at = to_utc_seconds(x[0], false);
                    end_at = to_utc_seconds(x[1], true);
                }
            }
        }
    }

    pwm::GetParam getGetParam(const option::OptionMap& map) {
        pwm::GetParam data;
        if (auto temp = map.use(od_service.name); temp) {
            data.service = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
        }
        if (auto temp = map.use(od_user.name); temp) {
            data.user = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
        }
        if (auto temp = map.use(od_name.name); temp) {
            data.name = std::bit_cast<char8_t*>(temp.as<std::string>().c_str());
        }
        if (auto temp = map.use(od_registered_at.name); temp) {
            auto ret = temp.as<std::vector<std::string>>();
            setup_datetime(data.begin_registered_at, data.end_registered_at, ret);
        }
        if (auto temp = map.use(od_update_at.name); temp) {
            auto ret = temp.as<std::vector<std::string>>();
            setup_datetime(data.begin_update_at, data.end_update_at, ret);
        }

        return data;
    }
}