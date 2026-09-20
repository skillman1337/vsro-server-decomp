/**
 * ============================================================================
 * Joymax BSLib - Date & Time Utilities
 * Original Source: D:\WORK2005\Source\JMX_Library\BSLib\Util\TimeSpan.h
 *
 * Implements native calendar and timestamp representations:
 *   - BSLib::CTime @ 0x009754A0 - 0x00975600
 * ============================================================================
 */

#ifndef _JMX_LIBRARY_BSLIB_UTIL_TIMESPAN_H_
#define _JMX_LIBRARY_BSLIB_UTIL_TIMESPAN_H_

#include <cstdint>
#include <ctime>
#include <cstring>

namespace BSLib {

/**
 * [RECONSTRUCTED - 0x009754A0 - 0x00975600]
 * BSLib::CTime
 * Exact struct size: 40 bytes (0x28)
 */
class CTime {
public:
    // Native 0x00975580: default constructor
    CTime();

    // Native constructor from time_t
    CTime(time_t t);

    // Native 0x00975600: Construct from date/time components
    CTime(int32_t nYear, int32_t nMonth, int32_t nDay, int32_t nHour, int32_t nMin, int32_t nSec);

    // [RECONSTRUCTED - 0x009754A0]
    void SetTime(time_t t);

    // [RECONSTRUCTED - 0x009754C0]
    time_t GetTime() const;

    // [RECONSTRUCTED - 0x009754D0]
    int32_t GetYear() const;

    // [RECONSTRUCTED - 0x009754E0]
    int32_t GetMonth() const;

    // [RECONSTRUCTED - 0x009754F0]
    int32_t GetDay() const;

    // [RECONSTRUCTED - 0x00975500]
    int32_t GetHour() const;

    // [RECONSTRUCTED - 0x00975510]
    int32_t GetMinute() const;

    // [RECONSTRUCTED - 0x00975520]
    int32_t GetSecond() const;

    // [RECONSTRUCTED - 0x00975580]
    void UpdateTm();

    // Operators
    inline bool operator<(const CTime& other) const { return m_time < other.m_time; }
    inline bool operator<=(const CTime& other) const { return m_time <= other.m_time; }
    inline bool operator>(const CTime& other) const { return m_time > other.m_time; }
    inline bool operator>=(const CTime& other) const { return m_time >= other.m_time; }
    inline bool operator==(const CTime& other) const { return m_time == other.m_time; }
    inline bool operator!=(const CTime& other) const { return m_time != other.m_time; }

public:
    // Exact struct layout matching native binary bytes:
    int32_t tm_sec;    // +0x00: Seconds after minute (0-59)
    int32_t tm_min;    // +0x04: Minutes after hour (0-59)
    int32_t tm_hour;   // +0x08: Hours since midnight (0-23)
    int32_t tm_mday;   // +0x0C: Day of month (1-31)
    int32_t tm_mon;    // +0x10: Months since January (0-11)
    int32_t tm_year;   // +0x14: Years since 1900
    int32_t tm_wday;   // +0x18: Days since Sunday (0-6)
    int32_t tm_yday;   // +0x1C: Days since January 1 (0-365)
    int32_t tm_isdst;  // +0x20: Daylight saving time flag
    time_t  m_time;    // +0x24: POSIX timestamp (seconds since Epoch)
};

} // namespace BSLib

#endif // _JMX_LIBRARY_BSLIB_UTIL_TIMESPAN_H_
