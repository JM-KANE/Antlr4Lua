#include "os.h"
#include "../State.h"
#include <chrono>
#include <sys/wait.h>

using namespace lua;
using namespace stdlib;
namespace t = std::chrono;
using namespace std::chrono_literals;

namespace
{
constexpr FuncReg<11> osFuncs{Reg{"clock", os::Clock},  {"date", os::Date},      {"difftime", os::Difftime},
                              {"execute", os::Execute}, {"exit", os::Exit},      {"getenv", os::Getenv},
                              {"remove", os::Remove},   {"rename", os::Rename},  {"setlocale", os::Setlocale},
                              {"time", os::Time},       {"tmpname", os::Tmpname}};

template <typename T>
auto formatTime(std::string_view fmt, const T& time)
{
    return std::vformat(fmt, std::make_format_args(time));
}
template <typename T>
auto getZoneTime(const T& time = t::system_clock::now())
{
    return t::zoned_time{t::current_zone(), time};
}
template <typename T>
auto getLocalTime(const T& time = t::system_clock::now())
{
    return getZoneTime(time).get_local_time();
}
template <typename T>
auto buildTimeTable(State* ls, const T& displayTime, uint8_t isdst)
{
    auto dp = t::floor<t::days>(displayTime);
    auto ymd = t::year_month_day(dp);
    auto hms = t::hh_mm_ss(displayTime - dp);
    t::weekday wd(dp);
    ls->CreateTable(0, 9);

    ls->PushInteger(static_cast<int>(ymd.year()));
    ls->SetField(-2, "year");
    ls->PushInteger(static_cast<unsigned>(ymd.month()));
    ls->SetField(-2, "month");
    ls->PushInteger(static_cast<unsigned>(ymd.day()));
    ls->SetField(-2, "day");
    ls->PushInteger(hms.hours().count());
    ls->SetField(-2, "hour");
    ls->PushInteger(hms.minutes().count());
    ls->SetField(-2, "min");
    ls->PushInteger(hms.seconds().count());
    ls->SetField(-2, "sec");
    ls->PushInteger(wd.c_encoding() + 1);
    ls->SetField(-2, "wday");
    auto year_start = t::year_month_day(ymd.year(), t::January, t::day(1));
    auto diff = t::sys_days{ymd} - t::sys_days{year_start};
    ls->PushInteger(diff.count() + 1);
    ls->SetField(-2, "yday");
    if (isdst < 0)
        ls->PushNil();
    else
        ls->PushBoolean(isdst > 0);
    ls->SetField(-2, "isdst");
}

}  // namespace

int32_t lua::stdlib::OpenOSLib(State* ls)
{
    ls->NewLib(osFuncs);
    return 1;
}

int32_t lua::stdlib::os::Clock(State* ls)
{
    auto clk = std::clock();
    auto res = (double)clk / CLOCKS_PER_SEC;
    ls->PushNumber(res);
    return 1;
}

int32_t lua::stdlib::os::Date(State* ls)
{
    auto fmt = ls->OptString(1, "%c");
    bool local = !ls->IsNoneOrNil(2);
    auto time = !local ? t::system_clock::now() : t::system_clock::time_point(t::seconds(ls->OptInteger(2, 0)));

    auto utc = fmt.front() == '!';
    if (utc)
    {
        fmt = fmt.substr(1);
    }
    if ("*t" == fmt)
    {
        uint8_t isdst = -1;
        auto displayTime = time;
        auto zone = utc ? t::locate_zone("Etc/UTC") : t::current_zone();
        if (zone)
        {
            auto info = zone->get_info(time);
            isdst = info.save != 0min ? 1 : 0;
            displayTime += info.save;
        }
        utc ? buildTimeTable(ls, displayTime, isdst) : buildTimeTable(ls, getLocalTime(displayTime), isdst);
    }
    else
    {
        std::string format_str = "{:" + fmt + "}";
        std::string result = utc ? formatTime(format_str, time) : formatTime(format_str, getZoneTime(time));
        ls->PushString(std::move(result));
    }
    return 1;
}

int32_t lua::stdlib::os::Difftime(State* ls)
{
    auto t2 = ls->CheckInteger(1);
    auto t1 = ls->CheckInteger(2);
    ls->PushInteger(t2 - t1);
    return 1;
}

int32_t lua::stdlib::os::Execute(State* ls)
{
    bool null = ls->IsNoneOrNil(1);
    auto strCmd = ls->CheckString(1);
    auto cmd = null ? nullptr : strCmd.c_str();
    ls->Pop(1);
    errno = 0;
    auto stat = system(cmd);
    if (cmd)
    {
        return ls->ExecResult(stat);
    }
    else
    {
        ls->PushBoolean(stat);
        return 1;
    }
}

int32_t lua::stdlib::os::Exit(State* ls)
{
    int stat;
    if (ls->IsBoolean(1))
        stat = ls->ToBoolean(1) ? EXIT_SUCCESS : EXIT_FAILURE;
    else
        stat = ls->OptInteger(1, EXIT_SUCCESS);
    if (ls->ToBoolean(2))
        /*TODO close state*/;
    exit(stat);
    return 0;
}

int32_t lua::stdlib::os::Getenv(State* ls)
{
    auto env = ls->CheckString(1);
    auto value = std::getenv(env.c_str());
    value ? ls->PushString(value) : ls->PushNil();
    return 1;
}

int32_t lua::stdlib::os::Remove(State* ls)
{
    return 0;
}

int32_t lua::stdlib::os::Rename(State* ls)
{
    return 0;
}

int32_t lua::stdlib::os::Setlocale(State* ls)
{
    return 0;
}

int32_t lua::stdlib::os::Time(State* ls)
{
    if (ls->IsNoneOrNil(1))
    {
        auto now = t::system_clock::now();
        auto seconds = t::floor<t::seconds>(now).time_since_epoch();
        ls->PushInteger(seconds.count());
    }
    else
    {
        ls->CheckType(1, cv::type::LUA_TTABLE);
        auto CheckDefaultField = [&](const char* field, int64_t d) -> int64_t
        {
            if (auto [i, ok] = ls->ToIntegerX(-1); ok)
            {
                ls->Pop(1);
                return i;
            }
            else
            {
                ls->Error2("field '%s' is not an integer", field);
                ls->Pop(1);
                return d;
            }
        };
        auto CheckField = [&](const char* field) -> int64_t
        {
            if (ls->IsNoneOrNil(-1))
            {
                ls->Error2("field '%s' missing in date table", field);
                ls->Pop(1);
                return -1;
            }
            return CheckDefaultField(field, -1);
        };
        ls->GetField(1, "year");
        t::year y{int(CheckField("year"))};
        ls->GetField(1, "month");
        t::month m{unsigned(CheckField("month"))};
        ls->GetField(1, "day");
        t::day d{unsigned(CheckField("day"))};
        ls->GetField(1, "hour");
        t::hours h{t::hours::rep(CheckDefaultField("hour", 12))};
        ls->GetField(1, "min");
        t::minutes min{t::minutes::rep(CheckDefaultField("min", 0))};
        ls->GetField(1, "sec");
        t::seconds sec{t::seconds::rep(CheckDefaultField("sec", 0))};
        t::local_days local_day(y / m / d);
        auto time = local_day + h + min + sec;
        auto sys_time = t::current_zone()->to_sys(time);
        ls->GetField(1, "isdst");
        auto isdst = ls->ToBoolean(-1);
        auto save = t::current_zone()->get_info(sys_time).save;
        sys_time -= save;
        ls->Pop(1);
        auto seconds = t::floor<t::seconds>(sys_time).time_since_epoch();
        ls->PushInteger(seconds.count());
    }

    return 1;
}

int32_t lua::stdlib::os::Tmpname(State* ls)
{
    return 0;
}
