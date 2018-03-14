// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    struct NamedAddress
    {
        NamedAddress()
            :   Address(L""),
                Name(L"")
        {
        }

        NamedAddress(std::wstring const & address)
            :   Address(address),
                Name(L"")
        {
        }

        NamedAddress(std::wstring const & address, std::wstring const & name)
            :   Address(address),
                Name(name)
        {
        }

        std::wstring Address;
        std::wstring Name;
    };
}
