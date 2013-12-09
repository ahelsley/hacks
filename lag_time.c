/* Lag Time                                                                                                                                                                                        
                                                                                                                                                                                                   
   Fool programs into thinking that the time is actually some other time
   relative to the current system clock.

   Usage:
    gcc -Wall -fPIC -ldl -shared -o lag_time.so lag_time.c
    LAG_TIME=-3600 LD_PRELOAD=$(pwd)/lag_time.so date

   Portions (c)2010 Dennis Kaarsemaker <dennis@kaarsemaker.net> (fake_time.c)

   The rest is (c) 2013 Andrew Helsley, attributed to the public domain and
   licensed under the Creative Commons Share-alike license where public domain
   licensing is not available.
*/
#include <stdlib.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <time.h>
#undef gettimeofday
#undef clock_gettime
#undef time
#undef ftime

#define _GNU_SOURCE
#define __USE_GNU
#include <dlfcn.h>

static time_t lag_time = 0;

typedef int (*original_clock_gettime_f)(clockid_t clk_id, struct timespec *tp);
original_clock_gettime_f original_clock_gettime = NULL;

static struct timespec get_time() {
    struct timespec tm = {tv_sec:0, tv_nsec:0};

    if (!original_clock_gettime) {
        /* Read the environment variable only once at program start
         *  (or every time in the case that dlsym is continually unable to locate
         * the original clock_gettime).
         */
        char *lag_delta = getenv("LAG_TIME");
        if (lag_delta) {
            char *error = NULL;
            lag_time = strtol(lag_delta, &error, 10);
            if (error && *error) {
                lag_time = 0;
            }
        }

        original_clock_gettime =
            (original_clock_gettime_f)dlsym(RTLD_NEXT, "clock_gettime");

        if(!original_clock_gettime) {
            return tm;
	}
    }

    if (0 == original_clock_gettime(CLOCK_REALTIME, &tm)) {
        tm.tv_sec += lag_time;
    }
    return tm;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    if(tv == NULL)
        return -1;
    /* //TODO// Hmmm, maybe something should be done with 'tz' here? */
    struct timespec tm = get_time();
    tv->tv_sec = tm.tv_sec;
    tv->tv_usec = (tm.tv_nsec / 1000);
    return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    if(tp == NULL)
        return -1;
    /* //TODO// Hmmm, maybe something should be done with 'clk_id' here? */
    struct timespec tm = get_time();
    tp->tv_sec = tm.tv_sec;
    tp->tv_nsec = tm.tv_nsec;
    return 0;
}

time_t time(time_t *t) {
    int t_ = get_time().tv_sec;
    if(t)
        *t = t_;
    return t_;
}

int ftime(struct timeb *tp) {
    tp->time = get_time().tv_sec;
    return 0;
}
