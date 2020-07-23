//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;

std::wstring GatewayResourceStatus::ToString(Enum const & val)
{
    wstring gatewayResourceStatus;
    StringWriter(gatewayResourceStatus).Write(val);
    return gatewayResourceStatus;
}

void GatewayResourceStatus::WriteToTextWriter(Common::TextWriter & w, Enum const & e)
{
    switch (e)
    {
    case Enum::Invalid: w << "Invalid"; return;
    case Enum::Creating: w << "Creating"; return;
    case Enum::Ready: w << "Ready"; return;
    case Enum::Deleting: w << "Deleting"; return;
    case Enum::Failed: w << "Failed"; return;
    case Enum::Upgrading: w << "Upgrading"; return;
    }
    w << "GatewayResourceStatus({0}" << static_cast<int>(e) << ')';
}

::FABRIC_GATEWAY_RESOURCE_STATUS GatewayResourceStatus::ToPublicApi(GatewayResourceStatus::Enum toConvert)
{
    switch (toConvert)
    {
    case Creating:
        return ::FABRIC_GATEWAY_RESOURCE_STATUS_CREATING;
    case Ready:
        return ::FABRIC_GATEWAY_RESOURCE_STATUS_READY;
    case Deleting:
        return ::FABRIC_GATEWAY_RESOURCE_STATUS_DELETING;
    case Failed:
        return ::FABRIC_GATEWAY_RESOURCE_STATUS_FAILED;
    case Upgrading:
        return ::FABRIC_GATEWAY_RESOURCE_STATUS_UPGRADING;
    case Invalid:
    default:
        return ::FABRIC_GATEWAY_RESOURCE_STATUS_INVALID;
    }
}

GatewayResourceStatus::Enum GatewayResourceStatus::FromPublicApi(FABRIC_GATEWAY_RESOURCE_STATUS toConvert)
{
    switch (toConvert)
    {
    case FABRIC_GATEWAY_RESOURCE_STATUS_CREATING:
        return Enum::Creating;
    case FABRIC_GATEWAY_RESOURCE_STATUS_READY:
        return Enum::Ready;
    case FABRIC_GATEWAY_RESOURCE_STATUS_DELETING:
        return Enum::Deleting;
    case FABRIC_GATEWAY_RESOURCE_STATUS_FAILED:
        return Enum::Failed;
    case FABRIC_GATEWAY_RESOURCE_STATUS_UPGRADING:
        return Enum::Upgrading;
    case FABRIC_GATEWAY_RESOURCE_STATUS_INVALID:
    default:
        return Enum::Invalid;
    }
}
