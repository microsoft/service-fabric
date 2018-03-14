// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
using namespace Naming;
using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

ResolvedServicePartitionWrapper::ResolvedServicePartitionWrapper(
    wstring &&serviceName,
    ServicePartitionInformation &&partitionInfo,
    vector<ServiceEndpointInformationWrapper> &&endpoints,
    wstring &&version)
    : serviceName_(move(serviceName))
    , partitionInfo_(move(partitionInfo))
    , endpoints_(move(endpoints))
    , version_(move(version))
{
}

ResolvedServicePartitionWrapper const& ResolvedServicePartitionWrapper::operator = (ResolvedServicePartitionWrapper &&other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
        partitionInfo_ = move(other.partitionInfo_);
        version_ = move(other.version_);
        endpoints_ = move(other.endpoints_);
    }

    return *this;
}
