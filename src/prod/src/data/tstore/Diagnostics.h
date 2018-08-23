// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {

// Note: This requires the class to have a member called assertTagLiteral_
#define STORE_ASSERT(condition, msg, ...)   { if (!(condition)) { Common::Assert::CodingError("{0} " msg, traceComponent_->AssertTag, ##__VA_ARGS__); } }

        class Diagnostics
        {
        public:
            static void Validate(NTSTATUS status);
            static void GetExceptionStackTrace(
                __in ktl::Exception const & exception,
                __out KDynStringA& stackString);

            static ULONG64 GetProcessMemoryUsageBytes();
        };

    }

}
