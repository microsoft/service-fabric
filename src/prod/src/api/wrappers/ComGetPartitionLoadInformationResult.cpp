// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Api
{
    ComGetPartitionLoadInformationResult::ComGetPartitionLoadInformationResult(
        PartitionLoadInformationQueryResult const & partitionLoadInformation)
        : partitionLoadInformation_()
        , heap_()
    {
        partitionLoadInformation_ = heap_.AddItem<FABRIC_PARTITION_LOAD_INFORMATION>();

        partitionLoadInformation.ToPublicApi(heap_, *partitionLoadInformation_);
    }

    const FABRIC_PARTITION_LOAD_INFORMATION *STDMETHODCALLTYPE ComGetPartitionLoadInformationResult::get_PartitionLoadInformation(void)
    {
        return partitionLoadInformation_.GetRawPointer();
    }
}
