#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Core/Global.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Serialization/ParamArchive.hpp>
#include <Poseidon/UI/Locale/StringtableExt.hpp>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

bool ObjViewer = false;

int MaxGroups;

ParamFile Pars;
ParamFile ExtParsCampaign;
ParamFile ExtParsMission;
ParamFile Res;
ParamFile Remaster;

namespace Poseidon
{

LSError Clock::Serialize(ParamArchive& ar)
{
    PARAM_CHECK(ar.Serialize("timeInYear", _timeInYear, 1));
    PARAM_CHECK(ar.Serialize("changeTime", _changeTime, 1));
    PARAM_CHECK(ar.Serialize("timeOfDay", _timeOfDay, 1));
    return LSOK;
}

bool Clock::AdvanceTime(float deltaT)
{
    _changeTime += deltaT;
    bool ret = fabs(_changeTime) > 20 * OneSecond;
    if (ret)
    {
        float oldTime = _timeInYear;
        _timeInYear += _changeTime;
        _changeTime -= (_timeInYear - oldTime);
    }
    const float InvOneDay = 1.0 / OneDay;
    _timeOfDay = fastFmod(_timeInYear, OneDay) * InvOneDay + _changeTime * InvOneDay;
    return ret;
}

static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int GetDaysInMonth(int year, int month)
{
    if (month == 1 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    {
        return 29;
    }
    else
    {
        return daysInMonth[month];
    }
}

// strftime-style date formatting that localizes the day-of-week (%a) and month (%b) via the
// stringtable instead of the C locale — so non-English UIs (and Cyrillic) render correctly.
// strftime's %a/%b always use the C locale (English), which is why dates stayed English.
void FormatLocalizedDate(const char* format, const struct tm& tmDate, char* buffer)
{
    const char* ptrsrc = format;
    char* ptrdst = buffer;
    while (*ptrsrc)
    {
        if (*ptrsrc == '%')
        {
            ptrsrc++;
            switch (*ptrsrc)
            {
                case 0:
                    break;
                case 'a':
                    strcpy(ptrdst, LocalizeString(IDS_SUNDAY + tmDate.tm_wday));
                    while (*ptrdst)
                    {
                        ptrdst++;
                    }
                    ptrsrc++;
                    break;
                case 'b':
                    strcpy(ptrdst, LocalizeString(IDS_JANUARY + tmDate.tm_mon));
                    while (*ptrdst)
                    {
                        ptrdst++;
                    }
                    ptrsrc++;
                    break;
                case 'd':
#ifdef _WIN32
                    _itoa(tmDate.tm_mday, ptrdst, 10);
                    while (*ptrdst)
                    {
                        ptrdst++;
                    }
#else
                    ptrdst += snprintf(ptrdst, sizeof(ptrdst), "%d", tmDate.tm_mday);
#endif
                    ptrsrc++;
                    break;
                case 'y':
#ifdef _WIN32
                    _itoa(tmDate.tm_year, ptrdst, 10);
                    while (*ptrdst)
                    {
                        ptrdst++;
                    }
#else
                    ptrdst += snprintf(ptrdst, sizeof(ptrdst), "%d", tmDate.tm_year);
#endif
                    ptrsrc++;
                    break;
                default:
                    ptrsrc++;
                    break;
            }
        }
        else
        {
            *(ptrdst++) = *(ptrsrc++);
        }
    }
    *(ptrdst++) = 0;
}

void Clock::FormatDate(const char* format, char* buffer)
{
    int dayOfYear = toIntFloor(_timeInYear * 365);
    if (_timeOfDay > 1.0)
    {
        dayOfYear++;
    }
    PoseidonAssert(dayOfYear >= 0 && dayOfYear < 365);
    struct tm tmDate = {0, 0, 0, 0, 0, 0};
    tmDate.tm_year = _year - 1900;
    tmDate.tm_yday = dayOfYear;
    int m = 0;
    while (tmDate.tm_yday >= GetDaysInMonth(_year, m))
    {
        tmDate.tm_yday -= GetDaysInMonth(_year, m);
        m++;
    }
    tmDate.tm_mday = tmDate.tm_yday + 1;
    tmDate.tm_mon = m;
    tmDate.tm_yday = 0;
    mktime(&tmDate);
    PoseidonAssert(tmDate.tm_yday == dayOfYear);
    FormatLocalizedDate(format, tmDate, buffer);
}

float Clock::ScanDateTime(const char* date, const char* time, int& year)
{
    int hour = 0, min = 0;
    int day = 0, month = 0;
    sscanf(time, "%d:%d", &hour, &min);
    sscanf(date, "%d/%d/%d", &day, &month, &year);
    day--;
    while (--month > 0)
    {
        day += GetDaysInMonth(year, month - 1);
    }
    return hour * OneHour + min * OneMinute + day * OneDay + 0.5 * OneSecond;
}

} // namespace Poseidon

void InitWorld() {}
