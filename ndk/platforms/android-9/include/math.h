/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $FreeBSD: src/lib/msun/src/math.h,v 1.61 2005/04/16 21:12:47 das Exp $
 */

#ifndef _MATH_H_
#define	_MATH_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <limits.h>

#define __pure2

/*
 * ANSI/POSIX
 */
extern const union __infinity_un {
	unsigned char	__uc[8];
	double		__ud;
} __infinity;

extern const union __nan_un {
	unsigned char	__uc[sizeof(float)];
	float		__uf;
} __nan;

/* #if __GNUC_PREREQ__(3, 3) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 800) */
#if 1
#define	__MATH_BUILTIN_CONSTANTS
#endif

/* #if __GNUC_PREREQ__(3, 0) && !defined(__INTEL_COMPILER) */
#if 1
#define	__MATH_BUILTIN_RELOPS
#endif

/* #ifdef __MATH_BUILTIN_CONSTANTS */
#if 1
#define	HUGE_VAL	__builtin_huge_val()
#else
#define	HUGE_VAL	(__infinity.__ud)
#endif

/* #if __ISO_C_VISIBLE >= 1999 */
#if 0
#define	FP_ILOGB0	(-__INT_MAX)
#define	FP_ILOGBNAN	__INT_MAX
#else
#define	FP_ILOGB0	(-INT_MAX)
#define	FP_ILOGBNAN	INT_MAX
#endif

#ifdef __MATH_BUILTIN_CONSTANTS
#define	HUGE_VALF	__builtin_huge_valf()
#define	HUGE_VALL	__builtin_huge_vall()
#define	INFINITY	__builtin_inf()
#define	NAN		__builtin_nan("")
#else
#define	HUGE_VALF	(float)HUGE_VAL
#define	HUGE_VALL	(long double)HUGE_VAL
#define	INFINITY	HUGE_VALF
#define	NAN		(__nan.__uf)
#endif /* __MATH_BUILTIN_CONSTANTS */

#define	MATH_ERRNO	1
#define	MATH_ERREXCEPT	2
#define	math_errhandling	MATH_ERREXCEPT

/* XXX We need a <machine/math.h>. */
#if defined(__ia64__) || defined(__sparc64__)
#define	FP_FAST_FMA
#endif
#ifdef __ia64__
#define	FP_FAST_FMAL
#endif
#define	FP_FAST_FMAF

/* Symbolic constants to classify floating point numbers. */
#define	FP_INFINITE	0x01
#define	FP_NAN		0x02
#define	FP_NORMAL	0x04
#define	FP_SUBNORMAL	0x08
#define	FP_ZERO		0x10

#if 0
typedef	__double_t	double_t;
typedef	__float_t	float_t;
#endif 
/* #endif */ /* __ISO_C_VISIBLE >= 1999 */

/*
 * XOPEN/SVID
 */
/* #if __BSD_VISIBLE || __XSI_VISIBLE */
#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#define	MAXFLOAT	((float)3.40282346638528860e+38)
extern int signgam;
/* #endif */ /* __BSD_VISIBLE || __XSI_VISIBLE */

#if __BSD_VISIBLE
#if 0
/* Old value from 4.4BSD-Lite math.h; this is probably better. */
#define	HUGE		HUGE_VAL
#else
#define	HUGE		MAXFLOAT
#endif
#endif /* __BSD_VISIBLE */

/*
 * Most of these functions depend on the rounding mode and have the side
 * effect of raising floating-point exceptions, so they are not declared
 * as __pure2.  In C99, FENV_ACCESS affects the purity of these functions.
 */
__BEGIN_DECLS
/*
 * ANSI/POSIX
 */
int	__fpclassifyd(double) __pure2;
int	__fpclassifyf(float) __pure2;
int	__fpclassifyl(long double) __pure2;
int	__isfinitef(float) __pure2;
int	__isfinite(double) __pure2;
int	__isfinitel(long double) __pure2;
int	__isinff(float) __pure2;
int     __isinf(double) __pure2;
int	__isinfl(long double) __pure2;
int	__isnanl(long double) __pure2;
int	__isnormalf(float) __pure2;
int	__isnormal(double) __pure2;
int	__isnormall(long double) __pure2;
int	__signbit(double) __pure2;
int	__signbitf(float) __pure2;
int	__signbitl(long double) __pure2;

double	acos(double);
double	asin(double);
double	atan(double);
double	atan2(double, double);
double	cos(double);
double	sin(double);
double	tan(double);

double	cosh(double);
double	sinh(double);
double	tanh(double);

double	exp(double);
double	frexp(double, int *);	/* fundamentally !__pure2 */
double	ldexp(double, int);
double	log(double);
double	log10(double);
double	modf(double, double *);	/* fundamentally !__pure2 */

double	pow(double, double);
double	sqrt(double);

double	ceil(double);
double	fabs(double) __pure2;
double	floor(double);
double	fmod(double, double);

/*
 * These functions are not in C90.
 */
/* #if __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __XSI_VISIBLE */
double	acosh(double);
double	asinh(double);
double	atanh(double);
double	cbrt(double);
double	erf(double);
double	erfc(double);
double	exp2(double);
double	expm1(double);
double	fma(double, double, double);
double	hypot(double, double);
int	ilogb(double) __pure2;
/* int	(isinf)(double) __pure2; */
int	(isnan)(double) __pure2;
double	lgamma(double);
long long llrint(double);
long long llround(double);
double	log1p(double);
double	logb(double);
long	lrint(double);
long	lround(double);
double	nextafter(double, double);
double	remainder(double, double);
double	remquo(double, double, int *);
double	rint(double);
/* #endif */ /* __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __XSI_VISIBLE */

/* #if __BSD_VISIBLE || __XSI_VISIBLE */
double	j0(double);
double	j1(double);
double	jn(int, double);
double	scalb(double, double);
double	y0(double);
double	y1(double);
double	yn(int, double);

/* #if __XSI_VISIBLE <= 500 || __BSD_VISIBLE */
double	gamma(double);
/* #endif */
/* #endif */ /* __BSD_VISIBLE || __XSI_VISIBLE */

/* #if __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 */
double	copysign(double, double) __pure2;
double	fdim(double, double);
double	fmax(double, double) __pure2;
double	fmin(double, double) __pure2;
double	nearbyint(double);
double	round(double);
double	scalbln(double, long);
double	scalbn(double, int);
double	tgamma(double);
double	trunc(double);
/* #endif */

/*
 * BSD math library entry points
 */
/* #if __BSD_VISIBLE */
double	drem(double, double);
int	finite(double) __pure2;
int	isnanf(float) __pure2;

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
double	gamma_r(double, int *);
double	lgamma_r(double, int *);

/*
 * IEEE Test Vector
 */
double	significand(double);
/* #endif */ /* __BSD_VISIBLE */

/* float versions of ANSI/POSIX functions */
/*#if __ISO_C_VISIBLE >= 1999 */
float	acosf(float);
float	asinf(float);
float	atanf(float);
float	atan2f(float, float);
float	cosf(float);
float	sinf(float);
float	tanf(float);

float	coshf(float);
float	sinhf(float);
float	tanhf(float);

float	exp2f(float);
float	expf(float);
float	expm1f(float);
float	frexpf(float, int *);	/* fundamentally !__pure2 */
int	ilogbf(float) __pure2;
float	ldexpf(float, int);
float	log10f(float);
float	log1pf(float);
float	logf(float);
float	modff(float, float *);	/* fundamentally !__pure2 */

float	powf(float, float);
float	sqrtf(float);

float	ceilf(float);
float	fabsf(float) __pure2;
float	floorf(float);
float	fmodf(float, float);
float	roundf(float);

float	erff(float);
float	erfcf(float);
float	hypotf(float, float);
float	lgammaf(float);

float	acoshf(float);
float	asinhf(float);
float	atanhf(float);
float	cbrtf(float);
float	logbf(float);
float	copysignf(float, float) __pure2;
long long llrintf(float);
long long llroundf(float);
long	lrintf(float);
long	lroundf(float);
float	nearbyintf(float);
float	nextafterf(float, float);
float	remainderf(float, float);
float	remquof(float, float, int *);
float	rintf(float);
float	scalblnf(float, long);
float	scalbnf(float, int);
float	truncf(float);

float	fdimf(float, float);
float	fmaf(float, float, float);
float	fmaxf(float, float) __pure2;
float	fminf(float, float) __pure2;
/* #endif */

/*
 * float versions of BSD math library entry points
 */
/* #if __BSD_VISIBLE */
float	dremf(float, float);
int	finitef(float) __pure2;
float	gammaf(float);
float	j0f(float);
float	j1f(float);
float	jnf(int, float);
float	scalbf(float, float);
float	y0f(float);
float	y1f(float);
float	ynf(int, float);

/*
 * Float versions of reentrant version of gamma & lgamma; passes
 * signgam back by reference as the second argument; user must
 * allocate space for signgam.
 */
float	gammaf_r(float, int *);
float	lgammaf_r(float, int *);

/*
 * float version of IEEE Test Vector
 */
float	significandf(float);
/* #endif */	/* __BSD_VISIBLE */ 

/*
 * long double versions of ISO/POSIX math functions
 */
/* #if __ISO_C_VISIBLE >= 1999 */
#if 0
long double	acoshl(long double);
long double	acosl(long double);
long double	asinhl(long double);
long double	asinl(long double);
long double	atan2l(long double, long double);
long double	atanhl(long double);
long double	atanl(long double);
long double	cbrtl(long double);
#endif
long double	ceill(long double);
long double	copysignl(long double, long double) __pure2;
#if 0
long double	coshl(long double);
long double	cosl(long double);
long double	erfcl(long double);
long double	erfl(long double);
long double	exp2l(long double);
long double	expl(long double);
long double	expm1l(long double);
#endif
long double	fabsl(long double) __pure2;
long double	fdiml(long double, long double);
long double	floorl(long double);
long double	fmal(long double, long double, long double);
long double	fmaxl(long double, long double) __pure2;
long double	fminl(long double, long double) __pure2;
#if 0
long double	fmodl(long double, long double);
#endif
long double	frexpl(long double value, int *); /* fundamentally !__pure2 */
#if 0
long double	hypotl(long double, long double);
#endif
int		ilogbl(long double) __pure2;
long double	ldexpl(long double, int);
#if 0
long double	lgammal(long double);
long long	llrintl(long double);
#endif
long long	llroundl(long double);
#if 0
long double	log10l(long double);
long double	log1pl(long double);
long double	log2l(long double);
long double	logbl(long double);
long double	logl(long double);
long		lrintl(long double);
#endif
long		lroundl(long double);
#if 0
long double	modfl(long double, long double *); /* fundamentally !__pure2 */
long double	nanl(const char *) __pure2;
long double	nearbyintl(long double);
#endif
long double	nextafterl(long double, long double);
double		nexttoward(double, long double);
float		nexttowardf(float, long double);
long double	nexttowardl(long double, long double);
#if 0
long double	powl(long double, long double);
long double	remainderl(long double, long double);
long double	remquol(long double, long double, int *);
long double	rintl(long double);
#endif
long double	roundl(long double);
long double	scalblnl(long double, long);
long double	scalbnl(long double, int);
#if 0
long double	sinhl(long double);
long double	sinl(long double);
long double	sqrtl(long double);
long double	tanhl(long double);
long double	tanl(long double);
long double	tgammal(long double);
#endif
long double	truncl(long double);

/* BIONIC: GLibc compatibility - required by the ARM toolchain */
#ifdef _GNU_SOURCE
void  sincos(double x, double *sin, double *cos);
void  sincosf(float x, float *sin, float *cos);
void  sincosl(long double x, long double *sin, long double *cos);
#endif

/* #endif */ /* __ISO_C_VISIBLE >= 1999 */
__END_DECLS

#if !defined(__cplusplus)
#define	fpclassify(x) \
    ((sizeof (x) == sizeof (float)) ? __fpclassifyf(x) \
    : (sizeof (x) == sizeof (double)) ? __fpclassifyd(x) \
    : __fpclassifyl(x))

#define	isfinite(x)					\
    ((sizeof (x) == sizeof (float)) ? __isfinitef(x)	\
    : (sizeof (x) == sizeof (double)) ? __isfinite(x)	\
    : __isfinitel(x))
#define	isinf(x)					\
    ((sizeof (x) == sizeof (float)) ? __isinff(x)	\
    : (sizeof (x) == sizeof (double)) ? __isinf(x)	\
    : __isinfl(x))
#define	isnan(x)					\
    ((sizeof (x) == sizeof (float)) ? isnanf(x)		\
    : (sizeof (x) == sizeof (double)) ? isnan(x)	\
    : __isnanl(x))
#define	isnormal(x)					\
    ((sizeof (x) == sizeof (float)) ? __isnormalf(x)	\
    : (sizeof (x) == sizeof (double)) ? __isnormal(x)	\
    : __isnormall(x))

#ifdef __MATH_BUILTIN_RELOPS
#define	isgreater(x, y)		__builtin_isgreater((x), (y))
#define	isgreaterequal(x, y)	__builtin_isgreaterequal((x), (y))
#define	isless(x, y)		__builtin_isless((x), (y))
#define	islessequal(x, y)	__builtin_islessequal((x), (y))
#define	islessgreater(x, y)	__builtin_islessgreater((x), (y))
#define	isunordered(x, y)	__builtin_isunordered((x), (y))
#else
#define	isgreater(x, y)		(!isunordered((x), (y)) && (x) > (y))
#define	isgreaterequal(x, y)	(!isunordered((x), (y)) && (x) >= (y))
#define	isless(x, y)		(!isunordered((x), (y)) && (x) < (y))
#define	islessequal(x, y)	(!isunordered((x), (y)) && (x) <= (y))
#define	islessgreater(x, y)	(!isunordered((x), (y)) && \
					((x) > (y) || (y) > (x)))
#define	isunordered(x, y)	(isnan(x) || isnan(y))
#endif /* __MATH_BUILTIN_RELOPS */

#define	signbit(x)					\
    ((sizeof (x) == sizeof (float)) ? __signbitf(x)	\
    : (sizeof (x) == sizeof (double)) ? __signbit(x)	\
    : __signbitl(x))

#else
/* In C++, don't use #define because the following functions
  may be #undef in cmath */

inline int fpclassify(float x) { return __fpclassifyf(x); }
inline int fpclassify(double x) { return __fpclassifyd(x); }
inline int fpclassify(long double x) { return __fpclassifyl(x); }

inline bool isfinite(float x) { return __isfinitef(x); }
inline bool isfinite(double x) { return __isfinite(x); }
inline bool isfinite(long double x) { return __isfinitel(x); }

inline bool isinf(float x) { return __isinff(x); }
inline bool isinf(double x) { return __isinf(x); }
inline bool isinf(long double x) { return __isinfl(x); }

inline bool isnan(float x) { return isnanf(x); }
/*inline bool isnan(double x) { return isnan(x); }*/
inline bool isnan(long double x) { return __isnanl(x); }

inline bool isnormal(float x) { return __isnormalf(x); }
inline bool isnormal(double x) { return __isnormal(x); }
inline bool isnormal(long double x) { return __isnormall(x); }

inline bool isgreater(float x, float y) { return __builtin_isgreater(x, y); }
inline bool isgreater(double x, double y) { return __builtin_isgreater(x, y); }
inline bool isgreater(long double x, long double y) { return __builtin_isgreater(x, y); }

inline bool isgreaterequal(float x, float y) { return __builtin_isgreaterequal(x, y); }
inline bool isgreaterequal(double x, double y) { return __builtin_isgreaterequal(x, y); }
inline bool isgreaterequal(long double x, long double y) { return __builtin_isgreaterequal(x, y); }

inline bool isless(float x, float y) { return __builtin_isless(x, y); }
inline bool isless(double x, double y) { return __builtin_isless(x, y); }
inline bool isless(long double x, long double y) { return __builtin_isless(x, y); }

inline bool islessequal(float x, float y) { return __builtin_islessequal(x, y); }
inline bool islessequal(double x, double y) { return __builtin_islessequal(x, y); }
inline bool islessequal(long double x, long double y) { return __builtin_islessequal(x, y); }

inline bool islessgreater(float x, float y) { return __builtin_islessgreater(x, y); }
inline bool islessgreater(double x, double y) { return __builtin_islessgreater(x, y); }
inline bool islessgreater(long double x, long double y) { return __builtin_islessgreater(x, y); }

inline bool isunordered(float x, float y) { return __builtin_isunordered(x, y); }
inline bool isunordered(double x, double y) { return __builtin_isunordered(x, y); }
inline bool isunordered(long double x, long double y) { return __builtin_isunordered(x, y); }

inline bool signbit(float x) { return __signbitf(x); }
inline bool signbit(double x) { return __signbit(x); }
inline bool signbit(long double x) { return __signbitl(x); }

#endif /* __cplusplus */

#endif /* !_MATH_H_ */
