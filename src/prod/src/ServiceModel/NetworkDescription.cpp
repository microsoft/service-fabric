// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("NetworkDescription");

NetworkDescription::NetworkDescription(wstring const & networkAddressPrefix)
    : networkType_(FABRIC_NETWORK_TYPE_LOCAL)
    , networkAddressPrefix_(move(networkAddressPrefix))
{
}

Common::ErrorCode NetworkDescription::FromPublicApi(__in FABRIC_NETWORK_DESCRIPTION const &networkDescription)
{
    if (networkDescription.NetworkType == FABRIC_NETWORK_TYPE_LOCAL)
    {
        auto localNetworkDescription = 
            reinterpret_cast<FABRIC_LOCAL_NETWORK_DESCRIPTION *>(networkDescription.Value);

        if (localNetworkDescription == nullptr)
        {
            Trace.WriteError(TraceComponent, "Local network description is not specified.");
            return ErrorCodeValue::InvalidArgument;
        }

        if (localNetworkDescription->NetworkConfiguration == nullptr)
        {
            Trace.WriteError(TraceComponent, "Network configuration for local network description is not specified.");
            return ErrorCodeValue::InvalidArgument;
        }

        auto error = StringUtility::LpcwstrToWstring2(localNetworkDescription->NetworkConfiguration->NetworkAddressPrefix, false, networkAddressPrefix_);
        if (!error.IsSuccess()) { return error; }
    }    
    else
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument);
    }

    return ErrorCode::Success();
}

void NetworkDescription::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1}", (int)networkType_, networkAddressPrefix_);
}