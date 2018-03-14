// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
//#include "ServiceModel/SystemServiceApplicationNameHelper.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

bool GetUnplacedReplicaInformationQuerySpecificationHelper::ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs)
{
    wstring serviceName;

    if (queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceName))
    {
        if (serviceName == SystemServiceApplicationNameHelper::PublicFMServiceName)
        {
            return true;
        }
    }
    return false;
}

wstring GetUnplacedReplicaInformationQuerySpecificationHelper::GetSpecificationId(QueryArgumentMap const& queryArgs, QueryNames::Enum const & queryName)
{
    return GetInternalSpecificationId(ShouldSendToFMM(queryArgs), queryName);
}

wstring GetUnplacedReplicaInformationQuerySpecificationHelper::GetInternalSpecificationId(bool sendToFMM, QueryNames::Enum const & queryName)
{
    if (sendToFMM)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(queryName), L"/FMM");
    }
    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(queryName), L"/FM");
}
