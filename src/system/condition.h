#ifndef __glhck_condition_h__
#define __glhck_condition_h__

#define _likely_(x)   __builtin_expect(!!(x), 1)
#define _unlikely_(x) __builtin_expect(!!(x), 0)

#endif /* __glhck_condition_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
