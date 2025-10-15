#define TRUE 1

#ifdef TRUE
#else
#error Should not reach
#endif

#if TRUE
#else
#error Should not reach
#endif

#if TRUE
#elif FALSE
#error Should not reach
#endif

#if TRUE
#elifdef FALSE
#error Should not reach
#endif

#if TRUE
#elifndef FALSE
#error Should not reach
#endif

#if TRUE
#elif TRUE
#error Should not reach
#endif

#if TRUE
#elifdef TRUE
#error Should not reach
#endif

#ifndef TRUE
#error Should not reach
#else
#endif

#ifndef TRUE
#error Should not reach
#elif TRUE
#else
#error Should not reach
#endif

#ifndef TRUE
#error Should not reach
#elifdef TRUE
#else
#error Should not reach
#endif

#ifndef TRUE
#error Should not reach
#elif FALSE
#error Should not reach
#else
#endif

#ifdef FALSE
#error Should not reach
#elifdef FALSE
#error Should not reach
#elifndef TRUE
#error Should not reach
#endif

