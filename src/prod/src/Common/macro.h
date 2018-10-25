// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DEFAULT_COPY_ASSIGNMENT( class_name ) public: class_name& operator = ( class_name const& ) = default;
#define DEFAULT_COPY_CONSTRUCTOR( class_name ) public: class_name( class_name const& ) = default;

#define DEFAULT_MOVE_ASSIGNMENT( class_name ) public: class_name& operator = ( class_name && ) = default;
#define DEFAULT_MOVE_CONSTRUCTOR( class_name ) public: class_name( class_name && ) = default;

#define DEFAULT_MOVE_AND_COPY( class_name ) \
    DEFAULT_COPY_ASSIGNMENT( class_name ) \
    DEFAULT_COPY_CONSTRUCTOR( class_name ) \
    DEFAULT_MOVE_ASSIGNMENT( class_name ) \
    DEFAULT_MOVE_CONSTRUCTOR( class_name ) \

#define DENY_COPY_ASSIGNMENT( class_name ) public: class_name& operator = ( class_name const& ) = delete;
#define DENY_COPY_CONSTRUCTOR( class_name ) public: class_name( class_name const& ) = delete;

#define DENY_COPY( class_name ) DENY_COPY_ASSIGNMENT( class_name ); DENY_COPY_CONSTRUCTOR( class_name )

#define COMMON_STRINGIFY( text ) COMMON_STRINGIFY_I( text )
#define COMMON_STRINGIFY_I( text ) #text

#define COMMON_WSTRINGIFY( text ) COMMON_WSTRINGIFY_I( text )
#define COMMON_WSTRINGIFY_I( text ) L ## #text

#define TO_WSTRING_I( x ) L##x
#define TO_WSTRING( x ) TO_WSTRING_I( x )

#define CPPBOOL(b) ( (b) ? true : false)
#define WINBOOL(b) ( (b) ? TRUE : FALSE)

#define FAIL_IF_RESERVED_FIELD_NOT_NULL(x) if (x->Reserved != NULL) \
    {\
        return Common::ComUtility::OnPublicApiReturn(\
            E_INVALIDARG, \
            GET_COMMON_RC(Reserved_Field_Not_Null));\
    }
#define FAIL_IF_OPTIONAL_PARAM_RESERVED_FIELD_NOT_NULL(x) if (x != NULL) { FAIL_IF_RESERVED_FIELD_NOT_NULL(x) }

#define ASSERT_IF(condition, ...)   { if (condition) { Common::Assert::CodingError(__VA_ARGS__); } }
#define ASSERT_IFNOT(condition, ...)   { if (!(condition)) { Common::Assert::CodingError(__VA_ARGS__); } }
#define CODING_ASSERT(...) { Common::Assert::CodingError(__VA_ARGS__); }
#define CODING_ERROR_ASSERT(expr) ASSERT_IFNOT(expr, COMMON_STRINGIFY(expr) " is false")
#define Invariant(expr) CODING_ERROR_ASSERT(expr)
#define Invariant2(expr, ...) { if (!(expr)) { Common::Assert::CodingError(__VA_ARGS__); } }
#define TRACE_AND_ASSERT(writer, type, id, ...) { writer(type, id, __VA_ARGS__); Common::Assert::CodingError(__VA_ARGS__); }
#define TRACE_LEVEL_AND_ASSERT(writer, type, ...) { writer(type, __VA_ARGS__); Common::Assert::CodingError(__VA_ARGS__); }
#define TRACE_ERROR_AND_ASSERT(type, ...) TRACE_LEVEL_AND_ASSERT(WriteError, type, __VA_ARGS__);

#define TESTASSERT_IF(condition, ...)   { if (condition) { Common::Assert::TestAssert(__VA_ARGS__); } }
#define TESTASSERT_IFNOT(condition, ...)   { if (!(condition)) { Common::Assert::TestAssert(__VA_ARGS__); } }
#define TRACE_AND_TESTASSERT(writer, type, id, ...) { writer(type, id, __VA_ARGS__); Common::Assert::TestAssert(__VA_ARGS__); }
#define TRACE_LEVEL_AND_TESTASSERT(writer, type, ...) { writer(type, __VA_ARGS__); Common::Assert::TestAssert(__VA_ARGS__); }
#define TRACE_ERROR_AND_TESTASSERT(type, ...) TRACE_LEVEL_AND_TESTASSERT(WriteError, type, __VA_ARGS__);
#define TRACE_WARNING_AND_TESTASSERT(type, ...) TRACE_LEVEL_AND_TESTASSERT(WriteWarning, type, __VA_ARGS__);
#define TRACE_INFO_AND_TESTASSERT(type, ...) TRACE_LEVEL_AND_TESTASSERT(WriteInfo, type, __VA_ARGS__);

#define ZeroRetValAssert(functionCall) \
{\
    auto retval = functionCall;\
    if (retval)\
    {\
        Common::Assert::CodingError(COMMON_STRINGIFY(functionCall) " failed: errno={0}", errno); \
    }\
}

#define GET_COMMON_RC(ResourceName) Common::StringResource::Get(IDS_COMMON_##ResourceName)
#define GET_NAMING_RC(ResourceName) Common::StringResource::Get(IDS_NAMING_##ResourceName)
#define GET_QUERY_RC(ResourceName) Common::StringResource::Get(IDS_QUERY_##ResourceName)
#define GET_HTTP_GATEWAY_RC(ResourceName) Common::StringResource::Get(IDS_HTTP_GATEWAY_##ResourceName)
#define GET_FSS_RC(ResourceName) Common::StringResource::Get(IDS_FSS_##ResourceName)

#if !defined(PLATFORM_UNIX)

#define OPERATOR_BOOL	operator bool() explicit const throw()
#define IMPLEMENTS(T, func)   T::func

// Windows threadpool implementation swallows stack overflow exceptions on threadpool threads. This is not a desirable
// behavior for us, so we crash the process on detecting that. Functions that have SEH(__try) cannot have objects with
// destructors, so this macro should be added to only functions that dont have C++ objects.

#define BEGIN_THREADPOOL_CALLBACK()                                                 \
            InterlockedIncrement(&Threadpool::ActiveCallbackCount());               \
            __try {

#define END_THREADPOOL_CALLBACK()                                                   \
                  }                                                                 \
            __except (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) \
                  {                                                                 \
                      ::RaiseFailFastException(NULL, NULL, 1);                      \
                  }                                                                 \
            InterlockedDecrement(&Threadpool::ActiveCallbackCount());

#else
#define OPERATOR_BOOL	explicit operator bool () const throw()
#define IMPLEMENTS(T, func)   TestExists<T>::Type::func

#define BEGIN_THREADPOOL_CALLBACK() InterlockedIncrement(&Threadpool::ActiveCallbackCount());

#define END_THREADPOOL_CALLBACK() InterlockedDecrement(&Threadpool::ActiveCallbackCount());

#endif
