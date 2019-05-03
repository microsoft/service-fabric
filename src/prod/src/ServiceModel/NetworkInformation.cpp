// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("NetworkInformation");

NetworkInformation::NetworkInformation()
    : networkType_(FABRIC_NETWORK_TYPE_INVALID)
    , networkName_()
    , networkAddressPrefix_()
    , networkStatus_(FABRIC_NETWORK_STATUS_INVALID)
{
}

NetworkInformation::NetworkInformation(NetworkInformation && other)
    : networkType_(move(other.networkType_))
    , networkName_(move(other.networkName_))
    , networkAddressPrefix_(move(other.networkAddressPrefix_))
    , networkStatus_(move(other.networkStatus_))
{
}

NetworkInformation::NetworkInformation(std::wstring const & networkName, 
    std::wstring const & networkAddressPrefix,
    FABRIC_NETWORK_TYPE networkType,
    FABRIC_NETWORK_STATUS networkStatus) 
    : networkType_(networkType)
    , networkName_(networkName)
    , networkAddressPrefix_(networkAddressPrefix)
    , networkStatus_(networkStatus)
{
}

NetworkInformation const & NetworkInformation::operator = (NetworkInformation && other)
{
    if (this != &other)
    {
        networkType_ = move(other.networkType_);
        networkName_ = move(other.networkName_);
        networkAddressPrefix_ = move(other.networkAddressPrefix_);
        networkStatus_ = move(other.networkStatus_);
    }

    return *this;
}

Common::ErrorCode NetworkInformation::FromPublicApi(__in FABRIC_NETWORK_INFORMATION const &networkInformation)
{    
    if (networkInformation.NetworkType == FABRIC_NETWORK_TYPE_LOCAL)
    {
        auto localNetworkInformation = 
            reinterpret_cast<FABRIC_LOCAL_NETWORK_INFORMATION *>(networkInformation.Value);

        if (localNetworkInformation == nullptr)
        {
            Trace.WriteError(TraceComponent, "Local network information is not specified.");
            return ErrorCodeValue::InvalidArgument;
        }

        auto error = StringUtility::LpcwstrToWstring2(localNetworkInformation->NetworkName, false, networkName_);
        if (!error.IsSuccess()) { return error; }

        if (localNetworkInformation->NetworkConfiguration == nullptr)
        {
            Trace.WriteError(TraceComponent, "Network configuration for local network information is not specified.");
            return ErrorCodeValue::InvalidArgument;
        }

        error = StringUtility::LpcwstrToWstring2(localNetworkInformation->NetworkConfiguration->NetworkAddressPrefix, false, networkAddressPrefix_);
        if (!error.IsSuccess()) { return error; }

        networkStatus_ = localNetworkInformation->NetworkStatus;
    }    
    else
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode::Success();
}

void NetworkInformation::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_NETWORK_INFORMATION & publicNetworkInformation) const
{
    if (networkType_ == FABRIC_NETWORK_TYPE_LOCAL)
    {
        auto localNetworkInformation = heap.AddItem<FABRIC_LOCAL_NETWORK_INFORMATION>();

        localNetworkInformation->NetworkName = heap.AddString(networkName_);
        localNetworkInformation->NetworkStatus = networkStatus_;

        auto localNetworkConfiguration = heap.AddItem<FABRIC_LOCAL_NETWORK_CONFIGURATION_DESCRIPTION>();
        localNetworkConfiguration->NetworkAddressPrefix = heap.AddString(networkAddressPrefix_);

        localNetworkConfiguration->Reserved = nullptr;

        localNetworkInformation->NetworkConfiguration = localNetworkConfiguration.GetRawPointer();
        localNetworkInformation->Reserved = nullptr;

        publicNetworkInformation.NetworkType = FABRIC_NETWORK_TYPE_LOCAL;
        publicNetworkInformation.Value = localNetworkInformation.GetRawPointer();
    }
    else
    {
        publicNetworkInformation.NetworkType = FABRIC_NETWORK_TYPE_INVALID;
        publicNetworkInformation.Value = nullptr;
    }
}

void NetworkInformation::WriteTo(__in Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("NetworkInformation({0} : {1}, {2}, {3})", networkName_, networkType_, networkAddressPrefix_, networkStatus_);
}
