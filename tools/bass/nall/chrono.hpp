#pragma once

#include <nall/function.hpp>
#include <nall/string.hpp>
#include "platform.hpp"

#if defined(PLATFORM_WINDOWS)
#define CLOCK_MONOTONIC 0
#define exp7           10000000i64     //1E+7     //C-file part
#define exp9         1000000000i64     //1E+9
#define w2ux 116444736000000000i64     //1.jan1601 to 1.jan1970
void unix_time(struct timespec *spec)
{  __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime); 
   wintime -=w2ux;  spec->tv_sec  =wintime / exp7;                 
                    spec->tv_nsec =wintime % exp7 *100;
}
int clock_gettime(int, timespec *spec)
{  static  struct timespec startspec; static double ticks2nano;
   static __int64 startticks, tps =0;    __int64 tmp, curticks;
   QueryPerformanceFrequency((LARGE_INTEGER*)&tmp); //some strange system can
   if (tps !=tmp) { tps =tmp; //init ~~ONCE         //possibly change freq ?
                    QueryPerformanceCounter((LARGE_INTEGER*)&startticks);
                    unix_time(&startspec); ticks2nano =(double)exp9 / tps; }
   QueryPerformanceCounter((LARGE_INTEGER*)&curticks); curticks -=startticks;
   spec->tv_sec  =startspec.tv_sec   +         (curticks / tps);
   spec->tv_nsec =startspec.tv_nsec  + (double)(curticks % tps) * ticks2nano;
         if (!(spec->tv_nsec < exp9)) { spec->tv_sec++; spec->tv_nsec -=exp9; }
   return 0;
}
#endif

namespace nall { namespace chrono { namespace {

//passage of time functions (from unknown epoch)

auto nanosecond() -> uint64_t {
  timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return tv.tv_sec * 1'000'000'000 + tv.tv_nsec;
}

auto microsecond() -> uint64_t { return nanosecond() / 1'000; }
auto millisecond() -> uint64_t { return nanosecond() / 1'000'000; }
auto second() -> uint64_t { return nanosecond() / 1'000'000'000; }

auto benchmark(const function<void ()>& f, uint64_t times = 1) -> void {
  auto start = nanosecond();
  while(times--) f();
  auto end = nanosecond();
  print("[chrono::benchmark] ", (double)(end - start) / 1'000'000'000.0, "s\n");
}

//exact date/time functions (from system epoch)

struct timeinfo {
  timeinfo(
    uint year = 0, uint month = 0, uint day = 0,
    uint hour = 0, uint minute = 0, uint second = 0, uint weekday = 0
  ) : year(year), month(month), day(day),
      hour(hour), minute(minute), second(second), weekday(weekday) {
  }

  explicit operator bool() const { return month; }

  uint year;     //...
  uint month;    //1 - 12
  uint day;      //1 - 31
  uint hour;     //0 - 23
  uint minute;   //0 - 59
  uint second;   //0 - 60
  uint weekday;  //0 - 6
};

auto timestamp() -> uint64_t {
  return ::time(nullptr);
}

namespace utc {
  auto timeinfo(uint64_t time = 0) -> chrono::timeinfo {
    auto stamp = time ? (time_t)time : (time_t)timestamp();
    auto info = gmtime(&stamp);
    return {
      (uint)info->tm_year + 1900,
      (uint)info->tm_mon + 1,
      (uint)info->tm_mday,
      (uint)info->tm_hour,
      (uint)info->tm_min,
      (uint)info->tm_sec,
      (uint)info->tm_wday
    };
  }

  auto year(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).year, 4, '0'); }
  auto month(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).month, 2, '0'); }
  auto day(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).day, 2, '0'); }
  auto hour(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).hour, 2, '0'); }
  auto minute(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).minute, 2, '0'); }
  auto second(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).second, 2, '0'); }

  auto date(uint64_t timestamp = 0) -> string {
    auto t = timeinfo(timestamp);
    return {pad(t.year, 4, '0'), "-", pad(t.month, 2, '0'), "-", pad(t.day, 2, '0')};
  }

  auto time(uint64_t timestamp = 0) -> string {
    auto t = timeinfo(timestamp);
    return {pad(t.hour, 2, '0'), ":", pad(t.minute, 2, '0'), ":", pad(t.second, 2, '0')};
  }

  auto datetime(uint64_t timestamp = 0) -> string {
    auto t = timeinfo(timestamp);
    return {
      pad(t.year, 4, '0'), "-", pad(t.month, 2, '0'), "-", pad(t.day, 2, '0'), " ",
      pad(t.hour, 2, '0'), ":", pad(t.minute, 2, '0'), ":", pad(t.second, 2, '0')
    };
  }
}

namespace local {
  auto timeinfo(uint64_t time = 0) -> chrono::timeinfo {
    auto stamp = time ? (time_t)time : (time_t)timestamp();
    auto info = localtime(&stamp);
    return {
      (uint)info->tm_year + 1900,
      (uint)info->tm_mon + 1,
      (uint)info->tm_mday,
      (uint)info->tm_hour,
      (uint)info->tm_min,
      (uint)info->tm_sec,
      (uint)info->tm_wday
    };
  }

  auto year(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).year, 4, '0'); }
  auto month(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).month, 2, '0'); }
  auto day(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).day, 2, '0'); }
  auto hour(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).hour, 2, '0'); }
  auto minute(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).minute, 2, '0'); }
  auto second(uint64_t timestamp = 0) -> string { return pad(timeinfo(timestamp).second, 2, '0'); }

  auto date(uint64_t timestamp = 0) -> string {
    auto t = timeinfo(timestamp);
    return {pad(t.year, 4, '0'), "-", pad(t.month, 2, '0'), "-", pad(t.day, 2, '0')};
  }

  auto time(uint64_t timestamp = 0) -> string {
    auto t = timeinfo(timestamp);
    return {pad(t.hour, 2, '0'), ":", pad(t.minute, 2, '0'), ":", pad(t.second, 2, '0')};
  }

  auto datetime(uint64_t timestamp = 0) -> string {
    auto t = timeinfo(timestamp);
    return {
      pad(t.year, 4, '0'), "-", pad(t.month, 2, '0'), "-", pad(t.day, 2, '0'), " ",
      pad(t.hour, 2, '0'), ":", pad(t.minute, 2, '0'), ":", pad(t.second, 2, '0')
    };
  }
}

}}}
