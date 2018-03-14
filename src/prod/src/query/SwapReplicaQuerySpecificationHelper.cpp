// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

bool SwapReplicaQuerySpecificationHelper::ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs)
{
    wstring partitionName;
    QueryAddressGenerator address = Query::QueryAddresses::GetFM();
    if (queryArgs.TryGetValue(QueryResourceProperties::Partition::PartitionId, partitionName))
    {
        if (partitionName == Reliability::Constants::FMServiceConsistencyUnitId.ToString())
        {
            return true;
        }
    }
    return false;
}

wstring SwapReplicaQuerySpecificationHelper::GetSpecificationId(QueryArgumentMap const& queryArgs, QueryNames::Enum const & queryName)
{
    return GetInternalSpecificationId(ShouldSendToFMM(queryArgs), queryName);
}

wstring SwapReplicaQuerySpecificationHelper::GetInternalSpecificationId(bool sendToFMM, QueryNames::Enum const & queryName)
{
    if (sendToFMM)
    {
        return MAKE_QUERY_SPEC_ID(QueryNames::ToString(queryName), L"/FMM");
    }
    return MAKE_QUERY_SPEC_ID(QueryNames::ToString(queryName), L"/FM");
}
