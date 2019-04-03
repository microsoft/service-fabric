// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(NetworkProperties)
INITIALIZE_SIZE_ESTIMATION(NetworkResourceDescription)

NetworkProperties::NetworkProperties(std::wstring const & networkAddressPrefix)
    : networkType_(FABRIC_NETWORK_TYPE_LOCAL)
    , networkAddressPrefix_(move(networkAddressPrefix))
{
}

bool NetworkProperties::operator==(NetworkProperties const & other) const
{
    if (networkType_ == other.networkType_
        && networkAddressPrefix_ == other.networkAddressPrefix_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode NetworkProperties::TryValidate(wstring const &traceId) const
{
    // Validate if the properties constitute a complete description for a local container network (or federated when it is added)
    if (networkType_ == FABRIC_NETWORK_TYPE_LOCAL)
    {
        if (networkAddressPrefix_.empty())
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(InvalidNetworkProperties), traceId, GetNetworkTypeString()));
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_MODELV2_RC(InvalidNetworkType), traceId, GetNetworkTypeString()));
}

void NetworkProperties::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("{0} {1}", GetNetworkTypeString(), networkAddressPrefix_);
}

std::wstring NetworkProperties::GetNetworkTypeString() const
{
    switch (networkType_)
    {
    case FABRIC_NETWORK_TYPE_INVALID:
        return L"Invalid";
    case FABRIC_NETWORK_TYPE_LOCAL:
        return L"Local";
    case FABRIC_NETWORK_TYPE_FEDERATED:
        return L"Federated";
    default:
        Assert::CodingError("Unknown network type value {0}", (int)networkType_);
    }
}

NetworkResourceDescription::NetworkResourceDescription(std::wstring const & networkName, ServiceModel::NetworkDescription const & networkDescription)
    : ResourceDescription(std::wstring(), move(networkName))
    , Properties(move(networkDescription.NetworkAddressPrefix))
{

}

bool NetworkResourceDescription::operator==(NetworkResourceDescription const & other) const
{
    if (ResourceDescription::operator==(other)
        && this->Properties == other.Properties)
    {
        return true;
    }
    else
    {
        return false;
    }
}

ErrorCode NetworkResourceDescription::TryValidate(wstring const &traceId) const
{
    ErrorCode error = __super::TryValidate(traceId);
    if (!error.IsSuccess())
    {
        return move(error);
    }

    return this->Properties.TryValidate(GetNextTraceId(traceId, Name));
}

void NetworkResourceDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "{0} [{1}]",
        Name,
        Properties);
}
