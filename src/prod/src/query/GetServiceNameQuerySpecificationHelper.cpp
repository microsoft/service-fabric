// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

wstring GetServiceNameQuerySpecificationHelper::GetSpecificationId(QueryArgumentMap const& queryArgs, QueryNames::Enum const & queryName)
{
    return GetInternalSpecificationId(ShouldSendToFMM(queryArgs), queryName);
}

bool GetServiceNameQuerySpecificationHelper::ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs)
{
    wstring partitionId;
    QueryAddressGenerator address = Query::QueryAddresses::GetFM();
    if (queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionId))
    {
        if (partitionId == Reliability::Constants::FMServiceConsistencyUnitId.ToString())
        {
            return true;
        }
    }
    return false;
}

wstring GetServiceNameQuerySpecificationHelper::GetInternalSpecificationId(bool sendToFMM, QueryNames::Enum const & queryName)
{
    if (sendToFMM)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(queryName), L"/FMM");
    }
    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(queryName), L"/FM");
}
