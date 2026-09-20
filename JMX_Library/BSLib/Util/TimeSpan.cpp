/**
 * ============================================================================
 * Joymax BSLib - Date & Time Utilities
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\Util\TimeSpan.cpp
 *
 * Implements native calendar and timestamp representations:
 *   - BSLib::CTime @ 0x009754A0 - 0x00975600
 * ============================================================================
 */

#include "TimeSpan.h"

namespace BSLib {

CTime::CTime() {
    std::memset(this, 0, sizeof(CTime));
}

CTime::CTime(time_t t) {
    std::memset(this, 0, sizeof(CTime));
    SetTime(t);
}

// [RECONSTRUCTED - 0x00975600]
CTime::CTime(int32_t nYear, int32_t nMonth, int32_t nDay, int32_t nHour, int32_t nMin, int32_t nSec) {
    std::memset(this, 0, sizeof(CTime));
    tm_year = nYear - 1900;
    tm_mon = nMonth - 1;
    tm_mday = nDay;
    tm_hour = nHour;
    tm_min = nMin;
    tm_sec = nSec;
    tm_isdst = -1;
    struct tm tmp = *reinterpret_cast<struct tm*>(this);
    m_time = ::mktime(&tmp);
    UpdateTm();
}

// [RECONSTRUCTED - 0x009754A0]
void CTime::SetTime(time_t t) {
    m_time = t;
    UpdateTm();
}

// [RECONSTRUCTED - 0x009754C0]
time_t CTime::GetTime() const {
    return m_time;
}

// [RECONSTRUCTED - 0x009754D0]
int32_t CTime::GetYear() const {
    return tm_year + 1900;
}

// [RECONSTRUCTED - 0x009754E0]
int32_t CTime::GetMonth() const {
    return tm_mon + 1;
}

// [RECONSTRUCTED - 0x009754F0]
int32_t CTime::GetDay() const {
    return tm_mday;
}

// [RECONSTRUCTED - 0x00975500]
int32_t CTime::GetHour() const {
    return tm_hour;
}

// [RECONSTRUCTED - 0x00975510]
int32_t CTime::GetMinute() const {
    return tm_min;
}

// [RECONSTRUCTED - 0x00975520]
int32_t CTime::GetSecond() const {
    return tm_sec;
}

// [RECONSTRUCTED - 0x00975580]
void CTime::UpdateTm() {
    if (m_time == 0) {
        std::memset(this, 0, 0x24);
        return;
    }
#ifdef _WIN32
    struct tm tmp;
    if (::localtime_s(&tmp, &m_time) == 0) {
        std::memcpy(this, &tmp, sizeof(struct tm));
    }
#else
    struct tm tmp;
    if (::localtime_r(&m_time, &tmp) != nullptr) {
        std::memcpy(this, &tmp, sizeof(struct tm));
    }
#endif
}

} // namespace BSLib
