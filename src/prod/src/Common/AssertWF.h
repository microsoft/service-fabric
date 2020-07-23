// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class Assert
    {
        DENY_COPY(Assert)

    public:
        static __declspec(noreturn) void CodingError(
            StringLiteral format
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6,
            VariableArgument const & arg7
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6,
            VariableArgument const & arg7,
            VariableArgument const & arg8
            );

        static __declspec(noreturn) void CodingError(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6,
            VariableArgument const & arg7,
            VariableArgument const & arg8,
            VariableArgument const & arg9
            );

        static void TestAssert(
            StringLiteral format = ""
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6,
            VariableArgument const & arg7
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6,
            VariableArgument const & arg7,
            VariableArgument const & arg8
            );

        static void TestAssert(
            StringLiteral format,
            VariableArgument const & arg0,
            VariableArgument const & arg1,
            VariableArgument const & arg2,
            VariableArgument const & arg3,
            VariableArgument const & arg4,
            VariableArgument const & arg5,
            VariableArgument const & arg6,
            VariableArgument const & arg7,
            VariableArgument const & arg8,
            VariableArgument const & arg9
            );

    public:
        class DisableDebugBreakInThisScope
        {
            DENY_COPY(DisableDebugBreakInThisScope)
        public:
            DisableDebugBreakInThisScope();
            ~DisableDebugBreakInThisScope();

        private:
            bool saved_;
        };

        class DisableTestAssertInThisScope
        {
            DENY_COPY(DisableTestAssertInThisScope)
        public:
            DisableTestAssertInThisScope();
            ~DisableTestAssertInThisScope();

        private:
            bool saved_;
        };

        static bool IsDebugBreakEnabled();
        static bool IsTestAssertEnabled();
        static bool IsStackTraceCaptureEnabled();
        static void LoadConfiguration(Common::Config & config);
        
        static void SetCrashLeasingApplicationCallback(void(*callback) (void));

    private:
        static __declspec(noreturn) void DoFailFast(std::string const & message);
        static void DoTestAssert(std::string const & message);

        static bool * static_TestAssertEnabled();
        static bool * static_DebugBreakEnabled();
        static bool * static_StackTraceCaptureEnabled();

        static inline void set_DebugBreakEnabled(bool value);
        static inline void set_TestAssertEnabled(bool value);
        static inline void set_StackTraceCaptureEnabled(bool value);
        static inline bool get_DebugBreakEnabled();
        static inline bool get_TestAssertEnabled();
        static inline bool get_StackTraceCaptureEnabled();
    };
}
