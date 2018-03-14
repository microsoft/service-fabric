// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Reliability;

    NamingServiceCuidCollection::NamingServiceCuidCollection(size_t partitionCount)
    {     
        ULONG count = static_cast<ULONG>(partitionCount);

        auto const & namingIdRange = *ConsistencyUnitId::NamingIdRange;
        ULONG reservedCount = namingIdRange.Count;
        if (partitionCount > reservedCount)
        {
            Trace.WriteWarning(
                Constants::NamingServiceSource,
                "Truncated naming store service partition count to maximum allowable: {0}.  Specified count was {1}.",
                reservedCount,
                partitionCount);
            count = namingIdRange.Count;
        }

        for (ULONG i = 0; i < count; ++i)
        {
            namingServiceCuids_.push_back(ConsistencyUnitId::CreateReserved(namingIdRange, i));
        }
    }

    ConsistencyUnitId NamingServiceCuidCollection::HashToNameOwner(NamingUri const & namingUri) const
    {
        size_t cuidsCount = namingServiceCuids_.size();
        if (cuidsCount <= 0)
        {
            Assert::CodingError("NamingService CUIDs have not been set.");
        }

        size_t key = namingUri.GetTrimQueryAndFragmentHash();

        ConsistencyUnitId owningPartition = namingServiceCuids_[key % cuidsCount];

        Trace.WriteNoise(
            Constants::NamingServiceSource, 
            "Hashed {0} to {1}: partition key = {2} CUIDs count = {3}", 
            namingUri, 
            owningPartition, 
            key, 
            cuidsCount);

        return owningPartition;
    }
}
