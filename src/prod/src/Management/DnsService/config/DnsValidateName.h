// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    enum DNSNAME_STATUS : USHORT
    {
        DNSNAME_VALID,
        DNSNAME_NON_RFC_NAME,
        DNSNAME_INVALID_NAME_CHAR,
        DNSNAME_INVALID_NAME
    };

    DNSNAME_STATUS IsDnsNameValid(
        __in LPCWSTR wszName
    );
}
