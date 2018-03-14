// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef _LTTNG_TRACEPOINT_H
#define _LTTNG_TRACEPOINT_H

#include <stdio.h>
#include <stdlib.h>
#include <lttng/tracepoint-types.h>
#include <lttng/tracepoint-rcu.h>
#include <urcu/compiler.h>
#include <urcu/system.h>
#include <dlfcn.h>	/* for dlopen */
#include <string.h>	/* for memset */
#include <lttng/ust-config.h>	/* for sdt */
#include <lttng/ust-compiler.h>

#ifdef LTTNG_UST_HAVE_SDT_INTEGRATION
#define SDT_USE_VARIADIC
#include <sys/sdt.h>
#define LTTNG_STAP_PROBEV STAP_PROBEV
#else
#define LTTNG_STAP_PROBEV(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define tracepoint_enabled(provider, name) \
	caa_unlikely(CMM_LOAD_SHARED(__tracepoint_##provider##___##name.state))

#define do_tracepoint(provider, name, ...) \
	__tracepoint_cb_##provider##___##name(__VA_ARGS__)

#define tracepoint(provider, name, ...)					    \
	do {								    \
		LTTNG_STAP_PROBEV(provider, name, ## __VA_ARGS__);	    \
		if (tracepoint_enabled(provider, name)) 		    \
			do_tracepoint(provider, name, __VA_ARGS__);	    \
	} while (0)

#define TP_ARGS(...)       __VA_ARGS__

/*
 * TP_ARGS takes tuples of type, argument separated by a comma.
 * It can take up to 10 tuples (which means that less than 10 tuples is
 * fine too).
 * Each tuple is also separated by a comma.
 */
#define __TP_COMBINE_TOKENS(_tokena, _tokenb)				\
		_tokena##_tokenb
#define _TP_COMBINE_TOKENS(_tokena, _tokenb)				\
		__TP_COMBINE_TOKENS(_tokena, _tokenb)
#define __TP_COMBINE_TOKENS3(_tokena, _tokenb, _tokenc)			\
		_tokena##_tokenb##_tokenc
#define _TP_COMBINE_TOKENS3(_tokena, _tokenb, _tokenc)			\
		__TP_COMBINE_TOKENS3(_tokena, _tokenb, _tokenc)
#define __TP_COMBINE_TOKENS4(_tokena, _tokenb, _tokenc, _tokend)	\
		_tokena##_tokenb##_tokenc##_tokend
#define _TP_COMBINE_TOKENS4(_tokena, _tokenb, _tokenc, _tokend)		\
		__TP_COMBINE_TOKENS4(_tokena, _tokenb, _tokenc, _tokend)

/*
 * _TP_EXVAR* extract the var names.
 * _TP_EXVAR1 and _TP_EXDATA_VAR1 are needed for -std=c99.
 */
#define _TP_EXVAR0()
#define _TP_EXVAR1(a)
#define _TP_EXVAR2(a,b)						b
#define _TP_EXVAR4(a,b,c,d)					b,d
#define _TP_EXVAR6(a,b,c,d,e,f)					b,d,f
#define _TP_EXVAR8(a,b,c,d,e,f,g,h)				b,d,f,h
#define _TP_EXVAR10(a,b,c,d,e,f,g,h,i,j)			b,d,f,h,j
#define _TP_EXVAR12(a,b,c,d,e,f,g,h,i,j,k,l)			b,d,f,h,j,l
#define _TP_EXVAR14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)		b,d,f,h,j,l,n
#define _TP_EXVAR16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)		b,d,f,h,j,l,n,p
#define _TP_EXVAR18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)	b,d,f,h,j,l,n,p,r
#define _TP_EXVAR20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)	b,d,f,h,j,l,n,p,r,t

#define _TP_EXDATA_VAR0()						__tp_data
#define _TP_EXDATA_VAR1(a)						__tp_data
#define _TP_EXDATA_VAR2(a,b)						__tp_data,b
#define _TP_EXDATA_VAR4(a,b,c,d)					__tp_data,b,d
#define _TP_EXDATA_VAR6(a,b,c,d,e,f)					__tp_data,b,d,f
#define _TP_EXDATA_VAR8(a,b,c,d,e,f,g,h)				__tp_data,b,d,f,h
#define _TP_EXDATA_VAR10(a,b,c,d,e,f,g,h,i,j)				__tp_data,b,d,f,h,j
#define _TP_EXDATA_VAR12(a,b,c,d,e,f,g,h,i,j,k,l)			__tp_data,b,d,f,h,j,l
#define _TP_EXDATA_VAR14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)			__tp_data,b,d,f,h,j,l,n
#define _TP_EXDATA_VAR16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)		__tp_data,b,d,f,h,j,l,n,p
#define _TP_EXDATA_VAR18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)		__tp_data,b,d,f,h,j,l,n,p,r
#define _TP_EXDATA_VAR20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)	__tp_data,b,d,f,h,j,l,n,p,r,t

/*
 * _TP_EXPROTO* extract tuples of type, var.
 * _TP_EXPROTO1 and _TP_EXDATA_PROTO1 are needed for -std=c99.
 */
#define _TP_EXPROTO0()						void
#define _TP_EXPROTO1(a)						void
#define _TP_EXPROTO2(a,b)					a b
#define _TP_EXPROTO4(a,b,c,d)					a b,c d
#define _TP_EXPROTO6(a,b,c,d,e,f)				a b,c d,e f
#define _TP_EXPROTO8(a,b,c,d,e,f,g,h)				a b,c d,e f,g h
#define _TP_EXPROTO10(a,b,c,d,e,f,g,h,i,j)			a b,c d,e f,g h,i j
#define _TP_EXPROTO12(a,b,c,d,e,f,g,h,i,j,k,l)			a b,c d,e f,g h,i j,k l
#define _TP_EXPROTO14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)		a b,c d,e f,g h,i j,k l,m n
#define _TP_EXPROTO16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)		a b,c d,e f,g h,i j,k l,m n,o p
#define _TP_EXPROTO18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)	a b,c d,e f,g h,i j,k l,m n,o p,q r
#define _TP_EXPROTO20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)	a b,c d,e f,g h,i j,k l,m n,o p,q r,s t

#define _TP_EXDATA_PROTO0()						void *__tp_data
#define _TP_EXDATA_PROTO1(a)						void *__tp_data
#define _TP_EXDATA_PROTO2(a,b)						void *__tp_data,a b
#define _TP_EXDATA_PROTO4(a,b,c,d)					void *__tp_data,a b,c d
#define _TP_EXDATA_PROTO6(a,b,c,d,e,f)					void *__tp_data,a b,c d,e f
#define _TP_EXDATA_PROTO8(a,b,c,d,e,f,g,h)				void *__tp_data,a b,c d,e f,g h
#define _TP_EXDATA_PROTO10(a,b,c,d,e,f,g,h,i,j)				void *__tp_data,a b,c d,e f,g h,i j
#define _TP_EXDATA_PROTO12(a,b,c,d,e,f,g,h,i,j,k,l)			void *__tp_data,a b,c d,e f,g h,i j,k l
#define _TP_EXDATA_PROTO14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)			void *__tp_data,a b,c d,e f,g h,i j,k l,m n
#define _TP_EXDATA_PROTO16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)		void *__tp_data,a b,c d,e f,g h,i j,k l,m n,o p
#define _TP_EXDATA_PROTO18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)		void *__tp_data,a b,c d,e f,g h,i j,k l,m n,o p,q r
#define _TP_EXDATA_PROTO20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)	void *__tp_data,a b,c d,e f,g h,i j,k l,m n,o p,q r,s t

/* Preprocessor trick to count arguments. Inspired from sdt.h. */
#define _TP_NARGS(...)			__TP_NARGS(__VA_ARGS__, 20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define __TP_NARGS(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, N, ...)	N
#define _TP_PROTO_N(N, ...)		_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXPROTO, N)(__VA_ARGS__))
#define _TP_VAR_N(N, ...)		_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXVAR, N)(__VA_ARGS__))
#define _TP_DATA_PROTO_N(N, ...)	_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXDATA_PROTO, N)(__VA_ARGS__))
#define _TP_DATA_VAR_N(N, ...)		_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXDATA_VAR, N)(__VA_ARGS__))
#define _TP_ARGS_PROTO(...)		_TP_PROTO_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#define _TP_ARGS_VAR(...)		_TP_VAR_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#define _TP_ARGS_DATA_PROTO(...)	_TP_DATA_PROTO_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#define _TP_ARGS_DATA_VAR(...)		_TP_DATA_VAR_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#define _TP_PARAMS(...)			__VA_ARGS__

/*
 * The tracepoint cb is marked always inline so we can distinguish
 * between caller's ip addresses within the probe using the return
 * address.
 */
#define _DECLARE_TRACEPOINT(_provider, _name, ...)			 		\
extern struct lttng_ust_tracepoint __tracepoint_##_provider##___##_name;		\
static inline __attribute__((always_inline, unused)) lttng_ust_notrace			\
void __tracepoint_cb_##_provider##___##_name(_TP_ARGS_PROTO(__VA_ARGS__));		\
static											\
void __tracepoint_cb_##_provider##___##_name(_TP_ARGS_PROTO(__VA_ARGS__))		\
{											\
	struct lttng_ust_tracepoint_probe *__tp_probe;						\
											\
	if (caa_unlikely(!TP_RCU_LINK_TEST()))						\
		return;									\
	tp_rcu_read_lock_bp();								\
	__tp_probe = tp_rcu_dereference_bp(__tracepoint_##_provider##___##_name.probes); \
	if (caa_unlikely(!__tp_probe))							\
		goto end;								\
	do {										\
		void (*__tp_cb)(void) = __tp_probe->func;				\
		void *__tp_data = __tp_probe->data;					\
											\
		URCU_FORCE_CAST(void (*)(_TP_ARGS_DATA_PROTO(__VA_ARGS__)), __tp_cb)	\
				(_TP_ARGS_DATA_VAR(__VA_ARGS__));			\
	} while ((++__tp_probe)->func);							\
end:											\
	tp_rcu_read_unlock_bp();							\
}											\
static inline lttng_ust_notrace								\
void __tracepoint_register_##_provider##___##_name(char *name,				\
		void (*func)(void), void *data);					\
static inline										\
void __tracepoint_register_##_provider##___##_name(char *name,				\
		void (*func)(void), void *data)						\
{											\
	__tracepoint_probe_register(name, func, data,					\
		__tracepoint_##_provider##___##_name.signature);			\
}											\
static inline lttng_ust_notrace								\
void __tracepoint_unregister_##_provider##___##_name(char *name,			\
		void (*func)(void), void *data);					\
static inline										\
void __tracepoint_unregister_##_provider##___##_name(char *name,			\
		void (*func)(void), void *data)						\
{											\
	__tracepoint_probe_unregister(name, func, data);				\
}

extern int __tracepoint_probe_register(const char *name, void (*func)(void),
		void *data, const char *signature);
extern int __tracepoint_probe_unregister(const char *name, void (*func)(void),
		void *data);

/*
 * tracepoint dynamic linkage handling (callbacks). Hidden visibility:
 * shared across objects in a module/main executable.
 */
struct lttng_ust_tracepoint_dlopen {
	void *liblttngust_handle;

	int (*tracepoint_register_lib)(struct lttng_ust_tracepoint * const *tracepoints_start,
		int tracepoints_count);
	int (*tracepoint_unregister_lib)(struct lttng_ust_tracepoint * const *tracepoints_start);
#ifndef _LGPL_SOURCE
	void (*rcu_read_lock_sym_bp)(void);
	void (*rcu_read_unlock_sym_bp)(void);
	void *(*rcu_dereference_sym_bp)(void *p);
#endif
};

extern struct lttng_ust_tracepoint_dlopen tracepoint_dlopen;

#if defined(TRACEPOINT_DEFINE) || defined(TRACEPOINT_CREATE_PROBES)

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the tracepoints per shared-ojbect
 * (or for the whole main program).
 */
int __tracepoint_registered
	__attribute__((weak, visibility("hidden")));
int __tracepoint_ptrs_registered
	__attribute__((weak, visibility("hidden")));
struct lttng_ust_tracepoint_dlopen tracepoint_dlopen
	__attribute__((weak, visibility("hidden")));

#ifndef _LGPL_SOURCE
static inline void lttng_ust_notrace
__tracepoint__init_urcu_sym(void);
static inline void
__tracepoint__init_urcu_sym(void)
{
	/*
	 * Symbols below are needed by tracepoint call sites and probe
	 * providers.
	 */
	if (!tracepoint_dlopen.rcu_read_lock_sym_bp)
		tracepoint_dlopen.rcu_read_lock_sym_bp =
			URCU_FORCE_CAST(void (*)(void),
				dlsym(tracepoint_dlopen.liblttngust_handle,
					"tp_rcu_read_lock_bp"));
	if (!tracepoint_dlopen.rcu_read_unlock_sym_bp)
		tracepoint_dlopen.rcu_read_unlock_sym_bp =
			URCU_FORCE_CAST(void (*)(void),
				dlsym(tracepoint_dlopen.liblttngust_handle,
					"tp_rcu_read_unlock_bp"));
	if (!tracepoint_dlopen.rcu_dereference_sym_bp)
		tracepoint_dlopen.rcu_dereference_sym_bp =
			URCU_FORCE_CAST(void *(*)(void *p),
				dlsym(tracepoint_dlopen.liblttngust_handle,
					"tp_rcu_dereference_sym_bp"));
}
#else
static inline void lttng_ust_notrace
__tracepoint__init_urcu_sym(void);
static inline void
__tracepoint__init_urcu_sym(void)
{
}
#endif

static void lttng_ust_notrace __attribute__((constructor))
__tracepoints__init(void);
static void
__tracepoints__init(void)
{
	if (__tracepoint_registered++)
		return;

	if (!tracepoint_dlopen.liblttngust_handle)
		tracepoint_dlopen.liblttngust_handle =
			dlopen("liblttng-ust-tracepoint.so.0", RTLD_NOW | RTLD_GLOBAL);
	if (!tracepoint_dlopen.liblttngust_handle)
		return;
	__tracepoint__init_urcu_sym();
}

// static void lttng_ust_notrace __attribute__((destructor))
// __tracepoints__destroy(void);
// static void
// __tracepoints__destroy(void)
// {
// 	int ret;

// 	if (--__tracepoint_registered)
// 		return;
// 	if (tracepoint_dlopen.liblttngust_handle && !__tracepoint_ptrs_registered) {
// 		ret = dlclose(tracepoint_dlopen.liblttngust_handle);
// 		if (ret) {
// 			fprintf(stderr, "Error (%d) in dlclose\n", ret);
// 			abort();
// 		}
// 		memset(&tracepoint_dlopen, 0, sizeof(tracepoint_dlopen));
// 	}
// }

#endif

#ifdef TRACEPOINT_DEFINE

/*
 * These weak symbols, the constructor, and destructor take care of
 * registering only _one_ instance of the tracepoints per shared-ojbect
 * (or for the whole main program).
 */
extern struct lttng_ust_tracepoint * const __start___tracepoints_ptrs[]
	__attribute__((weak, visibility("hidden")));
extern struct lttng_ust_tracepoint * const __stop___tracepoints_ptrs[]
	__attribute__((weak, visibility("hidden")));

/*
 * When TRACEPOINT_PROBE_DYNAMIC_LINKAGE is defined, we do not emit a
 * unresolved symbol that requires the provider to be linked in. When
 * TRACEPOINT_PROBE_DYNAMIC_LINKAGE is not defined, we emit an
 * unresolved symbol that depends on having the provider linked in,
 * otherwise the linker complains. This deals with use of static
 * libraries, ensuring that the linker does not remove the provider
 * object from the executable.
 */
#ifdef TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#define _TRACEPOINT_UNDEFINED_REF(provider)	NULL
#else	/* TRACEPOINT_PROBE_DYNAMIC_LINKAGE */
#define _TRACEPOINT_UNDEFINED_REF(provider)	&__tracepoint_provider_##provider
#endif /* TRACEPOINT_PROBE_DYNAMIC_LINKAGE */

/*
 * Note: to allow PIC code, we need to allow the linker to update the pointers
 * in the __tracepoints_ptrs section.
 * Therefore, this section is _not_ const (read-only).
 */
#define _TP_EXTRACT_STRING(...)	#__VA_ARGS__

#define _DEFINE_TRACEPOINT(_provider, _name, _args)				\
	extern int __tracepoint_provider_##_provider; 				\
	static const char __tp_strtab_##_provider##___##_name[]			\
		__attribute__((section("__tracepoints_strings"))) =		\
			#_provider ":" #_name;					\
	struct lttng_ust_tracepoint __tracepoint_##_provider##___##_name	\
		__attribute__((section("__tracepoints"))) =			\
		{								\
			__tp_strtab_##_provider##___##_name,			\
			0,							\
			NULL,							\
			_TRACEPOINT_UNDEFINED_REF(_provider), 			\
			_TP_EXTRACT_STRING(_args),				\
			{ },							\
		};								\
	static struct lttng_ust_tracepoint *					\
		__tracepoint_ptr_##_provider##___##_name			\
		__attribute__((used, section("__tracepoints_ptrs"))) =		\
			&__tracepoint_##_provider##___##_name;

static void lttng_ust_notrace __attribute__((constructor))
__tracepoints__ptrs_init(void);
static void
__tracepoints__ptrs_init(void)
{
	if (__tracepoint_ptrs_registered++)
		return;
	if (!tracepoint_dlopen.liblttngust_handle)
		tracepoint_dlopen.liblttngust_handle =
			dlopen("liblttng-ust-tracepoint.so.0", RTLD_NOW | RTLD_GLOBAL);
	if (!tracepoint_dlopen.liblttngust_handle)
		return;
	tracepoint_dlopen.tracepoint_register_lib =
		URCU_FORCE_CAST(int (*)(struct lttng_ust_tracepoint * const *, int),
				dlsym(tracepoint_dlopen.liblttngust_handle,
					"tracepoint_register_lib"));
	tracepoint_dlopen.tracepoint_unregister_lib =
		URCU_FORCE_CAST(int (*)(struct lttng_ust_tracepoint * const *),
				dlsym(tracepoint_dlopen.liblttngust_handle,
					"tracepoint_unregister_lib"));
	__tracepoint__init_urcu_sym();
	if (tracepoint_dlopen.tracepoint_register_lib) {
		tracepoint_dlopen.tracepoint_register_lib(__start___tracepoints_ptrs,
				__stop___tracepoints_ptrs -
				__start___tracepoints_ptrs);
	}
}

// static void lttng_ust_notrace __attribute__((destructor))
// __tracepoints__ptrs_destroy(void);
// static void
// __tracepoints__ptrs_destroy(void)
// {
// 	int ret;

// 	if (--__tracepoint_ptrs_registered)
// 		return;
// 	if (tracepoint_dlopen.tracepoint_unregister_lib)
// 		tracepoint_dlopen.tracepoint_unregister_lib(__start___tracepoints_ptrs);
// 	if (tracepoint_dlopen.liblttngust_handle && !__tracepoint_registered) {
// 		ret = dlclose(tracepoint_dlopen.liblttngust_handle);
// 		if (ret) {
// 			fprintf(stderr, "Error (%d) in dlclose\n", ret);
// 			abort();
// 		}
// 		memset(&tracepoint_dlopen, 0, sizeof(tracepoint_dlopen));
// 	}
// }

#else /* TRACEPOINT_DEFINE */

#define _DEFINE_TRACEPOINT(_provider, _name, _args)

#endif /* #else TRACEPOINT_DEFINE */

#ifdef __cplusplus
}
#endif

#endif /* _LTTNG_TRACEPOINT_H */

/* The following declarations must be outside re-inclusion protection. */

#ifndef TRACEPOINT_EVENT

/*
 * How to use the TRACEPOINT_EVENT macro:
 *
 * An example:
 * 
 * TRACEPOINT_EVENT(someproject_component, event_name,
 *
 *     * TP_ARGS takes from 0 to 10 "type, field_name" pairs *
 *
 *     TP_ARGS(int, arg0, void *, arg1, char *, string, size_t, strlen,
 *             long *, arg4, size_t, arg4_len),
 *
 *	* TP_FIELDS describes the event payload layout in the trace *
 *
 *     TP_FIELDS(
 *         * Integer, printed in base 10 * 
 *         ctf_integer(int, field_a, arg0)
 *
 *         * Integer, printed with 0x base 16 * 
 *         ctf_integer_hex(unsigned long, field_d, arg1)
 *
 *         * Array Sequence, printed as UTF8-encoded array of bytes * 
 *         ctf_array_text(char, field_b, string, FIXED_LEN)
 *         ctf_sequence_text(char, field_c, string, size_t, strlen)
 *
 *         * String, printed as UTF8-encoded string * 
 *         ctf_string(field_e, string)
 *
 *         * Array sequence of signed integer values * 
 *         ctf_array(long, field_f, arg4, FIXED_LEN4)
 *         ctf_sequence(long, field_g, arg4, size_t, arg4_len)
 *     )
 * )
 *
 * More detailed explanation:
 *
 * The name of the tracepoint is expressed as a tuple with the provider
 * and name arguments.
 *
 * The provider and name should be a proper C99 identifier.
 * The "provider" and "name" MUST follow these rules to ensure no
 * namespace clash occurs:
 *
 * For projects (applications and libraries) for which an entity
 * specific to the project controls the source code and thus its
 * tracepoints (typically with a scope larger than a single company):
 *
 * either:
 *   project_component, event
 * or:
 *   project, event
 *
 * Where "project" is the name of the project,
 *       "component" is the name of the project component (which may
 *       include several levels of sub-components, e.g.
 *       ...component_subcomponent_...) where the tracepoint is located
 *       (optional),
 *       "event" is the name of the tracepoint event.
 *
 * For projects issued from a single company wishing to advertise that
 * the company controls the source code and thus the tracepoints, the
 * "com_" prefix should be used:
 *
 * either:
 *   com_company_project_component, event
 * or:
 *   com_company_project, event
 *
 * Where "company" is the name of the company,
 *       "project" is the name of the project,
 *       "component" is the name of the project component (which may
 *       include several levels of sub-components, e.g.
 *       ...component_subcomponent_...) where the tracepoint is located
 *       (optional),
 *       "event" is the name of the tracepoint event.
 *
 * the provider:event identifier is limited to 127 characters.
 */

#define TRACEPOINT_EVENT(provider, name, args, fields)			\
	_DECLARE_TRACEPOINT(provider, name, _TP_PARAMS(args))		\
	_DEFINE_TRACEPOINT(provider, name, _TP_PARAMS(args))

#define TRACEPOINT_EVENT_CLASS(provider, name, args, fields)

#define TRACEPOINT_EVENT_INSTANCE(provider, _template, name, args)	\
	_DECLARE_TRACEPOINT(provider, name, _TP_PARAMS(args))		\
	_DEFINE_TRACEPOINT(provider, name, _TP_PARAMS(args))

#endif /* #ifndef TRACEPOINT_EVENT */

#ifndef TRACEPOINT_LOGLEVEL

/*
 * Tracepoint Loglevels
 *
 * Typical use of these loglevels:
 *
 * The loglevels go from 0 to 14. Higher numbers imply the most
 * verbosity (higher event throughput expected.
 *
 * Loglevels 0 through 6, and loglevel 14, match syslog(3) loglevels
 * semantic. Loglevels 7 through 13 offer more fine-grained selection of
 * debug information.
 *
 * TRACE_EMERG           0
 * system is unusable
 *
 * TRACE_ALERT           1
 * action must be taken immediately
 *
 * TRACE_CRIT            2
 * critical conditions
 *
 * TRACE_ERR             3
 * error conditions
 *
 * TRACE_WARNING         4
 * warning conditions
 *
 * TRACE_NOTICE          5
 * normal, but significant, condition
 *
 * TRACE_INFO            6
 * informational message
 *
 * TRACE_DEBUG_SYSTEM    7
 * debug information with system-level scope (set of programs)
 *
 * TRACE_DEBUG_PROGRAM   8
 * debug information with program-level scope (set of processes)
 *
 * TRACE_DEBUG_PROCESS   9
 * debug information with process-level scope (set of modules)
 *
 * TRACE_DEBUG_MODULE    10
 * debug information with module (executable/library) scope (set of units)
 *
 * TRACE_DEBUG_UNIT      11
 * debug information with compilation unit scope (set of functions)
 *
 * TRACE_DEBUG_FUNCTION  12
 * debug information with function-level scope
 *
 * TRACE_DEBUG_LINE      13
 * debug information with line-level scope (TRACEPOINT_EVENT default)
 *
 * TRACE_DEBUG           14
 * debug-level message
 *
 * Declare tracepoint loglevels for tracepoints. A TRACEPOINT_EVENT
 * should be declared prior to the the TRACEPOINT_LOGLEVEL for a given
 * tracepoint name. The first field is the provider name, the second
 * field is the name of the tracepoint, the third field is the loglevel
 * name.
 *
 *      TRACEPOINT_LOGLEVEL(< [com_company_]project[_component] >, < event >,
 *              < loglevel_name >)
 *
 * The TRACEPOINT_PROVIDER must be already declared before declaring a
 * TRACEPOINT_LOGLEVEL.
 */

enum {
	TRACE_EMERG		= 0,
	TRACE_ALERT		= 1,
	TRACE_CRIT		= 2,
	TRACE_ERR		= 3,
	TRACE_WARNING		= 4,
	TRACE_NOTICE		= 5,
	TRACE_INFO		= 6,
	TRACE_DEBUG_SYSTEM	= 7,
	TRACE_DEBUG_PROGRAM	= 8,
	TRACE_DEBUG_PROCESS	= 9,
	TRACE_DEBUG_MODULE	= 10,
	TRACE_DEBUG_UNIT	= 11,
	TRACE_DEBUG_FUNCTION	= 12,
	TRACE_DEBUG_LINE	= 13,
	TRACE_DEBUG		= 14,
};

#define TRACEPOINT_LOGLEVEL(provider, name, loglevel)

#endif /* #ifndef TRACEPOINT_LOGLEVEL */

#ifndef TRACEPOINT_MODEL_EMF_URI

#define TRACEPOINT_MODEL_EMF_URI(provider, name, uri)

#endif /* #ifndef TRACEPOINT_MODEL_EMF_URI */
