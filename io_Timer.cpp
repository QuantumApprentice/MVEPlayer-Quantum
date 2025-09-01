#include <cstdint>
#include <time.h>
#include <stdio.h>

#ifdef WINDOWS
#include <Windows.h>
#include <string.h>

uint64_t io_nano_time()
{
     //timing code for WINDOWS ONLY
     LARGE_INTEGER StartingTime;
     LARGE_INTEGER Frequency;
     QueryPerformanceFrequency(&Frequency);
     QueryPerformanceCounter(&StartingTime);

     return StartingTime.QuadPart * (1'000'000'000ULL / Frequency.QuadPart);

    //return (clock() / CLOCKS_PER_SEC);
}

void io_print_timer(uint64_t StartingTime)
{
    ////timing code WINDOWS ONLY
    // QueryPerformanceCounter(&EndingTime);
    // ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    // ElapsedMicroseconds.QuadPart *= 1000000;
    // ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    // printf("wstring_view 1_line parent_path_size(a_v < b_v) time: %d\n", ElapsedMicroseconds.QuadPart);
    uint64_t EndingTime = nano_time();
    uint64_t nanoseconds_total = EndingTime - StartingTime; // = NANOSECONDS_IN_SECOND * (end.tv_sec - start.tv_sec);
    uint64_t microseconds_total = nanoseconds_total / 1000;
    printf("Total time elapsed: %ldus\n", microseconds_total);
}

#elif defined(LINUX)
#include <strings.h>
#include <sys/stat.h>

uint64_t io_nano_time()
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    uint64_t nanotime = (uint64_t)start.tv_sec * 1'000'000'000ULL;
    nanotime += start.tv_nsec;
    return nanotime;
    // return (start.tv_sec * 1000 + (start.tv_nsec)/(long)1000000);
}

void io_print_timer(uint64_t StartingTime)
{
        // timing code LINUX
        uint64_t EndingTime = io_nano_time();
        uint64_t nanoseconds_total = EndingTime - StartingTime; // = NANOSECONDS_IN_SECOND * (end.tv_sec - start.tv_sec);
        uint64_t microseconds_total = nanoseconds_total / 1000;
        printf("Total time elapsed: %ldμs\n", microseconds_total);
}
#endif


