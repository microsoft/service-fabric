// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace IpUtility
    {
        ErrorCode GetDnsServers(
            __out std::vector<std::wstring> & list
            );

        ErrorCode GetDnsServersPerAdapter(
            __out std::map<std::wstring, std::vector<std::wstring>> & map
            );

        ErrorCode GetIpAddressesPerAdapter(
            __out std::map<std::wstring, std::vector<Common::IPPrefix>> & map
            );

        ErrorCode GetAdapterAddressOnTheSameNetwork(
            __in std::wstring input,
            __out std::wstring& output
            );

        ErrorCode GetGatewaysPerAdapter(
            __out std::map<std::wstring, std::vector<std::wstring>> &map
            );

        ErrorCode GetFirstNonLoopbackAddress(
            std::map<std::wstring, std::vector<Common::IPPrefix>> addressesPerAdapter,
            __out wstring &nonLoopbackIp
            );
    }
}
