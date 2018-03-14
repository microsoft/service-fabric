// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"

namespace DNS
{
    class DnsPropertyValue :
        public Serialization::FabricSerializable
    {
    public:
        DnsPropertyValue() { }

        DnsPropertyValue(const std::wstring & name) : ServiceName(name) { }

        ~DnsPropertyValue() { }

        FABRIC_FIELDS_01(ServiceName);

    public:
        std::wstring ServiceName;
    };
}
