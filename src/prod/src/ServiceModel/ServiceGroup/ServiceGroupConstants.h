// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceGroup
{
    class ServiceGroupConstants
    {
    public:
        
        //
        // Tracing
        //
        static Common::StringLiteral const TraceSource;

        //
        // Service group
        //
        static Common::GlobalWString MemberServiceNameDelimiter;
        static Common::GlobalWString ServiceAddressEscapedDelimiter;
        static Common::GlobalWString ServiceAddressDoubleDelimiter;
        static Common::GlobalWString ServiceAddressDelimiter;
    };
}
