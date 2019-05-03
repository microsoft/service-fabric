// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

namespace Hosting2
{
    OverlayNetworkResource::OverlayNetworkResource(
        uint ipAddress,
        uint64 macAddress) :
        IPV4Address_(ipAddress),
        macAddress_(macAddress),
        reservationId_() 
    {
    }

    OverlayNetworkResource::OverlayNetworkResource(
        uint ipAddress,
        uint64 macAddress,
        wstring reservationId) :
        IPV4Address_(ipAddress),
        macAddress_(macAddress),
        reservationId_(reservationId) 
    {
    }

    OverlayNetworkResource::OverlayNetworkResource(
        wstring ipAddress,
        wstring macAddress) :
        reservationId_()
    {
        bool ipAddressParsed = Hosting2::IPHelper::TryParse(ipAddress, IPV4Address_);
        ASSERT_IF(!ipAddressParsed, "Ip address is invalid");

        bool macAddressParsed = Hosting2::MACHelper::TryParse(macAddress, macAddress_);
        ASSERT_IF(!macAddressParsed, "Mac address is invalid");
    }

    std::wstring OverlayNetworkResource::FormatAsString()
    {
        return wformatString("IP:{0}, MAC:{1}, ResId:{2}", IPHelper::Format(this->IPV4Address_), MACHelper::Format(this->macAddress_), this->reservationId_);
    }
}