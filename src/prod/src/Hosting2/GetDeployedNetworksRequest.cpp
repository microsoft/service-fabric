// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

GetDeployedNetworksRequest::GetDeployedNetworksRequest()
{

}

GetDeployedNetworksRequest::GetDeployedNetworksRequest(NetworkType::Enum networkType)
    : networkType_(networkType)
{
}

void GetDeployedNetworksRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetDeployedNetworksRequest { ");
    w.Write("NetworkType: {0} ", NetworkType::EnumToString(networkType_));
    w.Write("}");
}
