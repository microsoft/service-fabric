// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace TraceKeywords
    {
        // User-defined ETW keywords. Values must be powers of 2, starting with 0x1.
        enum Enum : ULONGLONG
        {
            Default           = 0x01,
            AppInstance       = 0x02,
            ForQuery          = 0x04,
            CustomerInfo      = 0x08,
            DataMessaging     = 0x10,
            DataMessagingAll  = 0x20,

            // These 4 bits are reserved for tests and is for differentiating between rings when tests are running in parallel
            // without this all events will go to all sessions. With this we can setup keyword masks to only have events for 
            // a specific test going to its sessions. 
            Test0 = 0x0000100000000000,
            Test1 = 0x0000200000000000,
            Test2 = 0x0000400000000000,
            Test3 = 0x0000800000000000,

            // Do not use the following values (reserved by ETW):
            // 0x0001000000000000
            // 0x0002000000000000
            // 0x0004000000000000
            // 0x0008000000000000
            // 0x0010000000000000
            // 0x0020000000000000
            // 0x0040000000000000
            // 0x0080000000000000
            // 0x0100000000000000
            // 0x0200000000000000
            // 0x0400000000000000
            // 0x0800000000000000
            // 0x1000000000000000
            // 0x2000000000000000
            // 0x4000000000000000
            // 0x8000000000000000

            OperationalChannel = 0x4000000000000000,

            All = 0xffffffffffffffff,
        };

        ULONGLONG ParseTestKeyword(std::wstring const& testKeyword);
    }

#define TRACE_KEYWORDS2(x,y) (Common::TraceKeywords::Enum)(Common::TraceKeywords::x | Common::TraceKeywords::y)
#define TRACE_KEYWORDS3(x,y,z) (Common::TraceKeywords::Enum)(Common::TraceKeywords::x | Common::TraceKeywords::y | Common::TraceKeywords::z)
}
