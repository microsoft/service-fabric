// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    DEFINE_SERIALIZATION_OVERHEAD_ESTIMATE( ServiceLocationNotificationReplyBody )

    ServiceLocationNotificationReplyBody::ServiceLocationNotificationReplyBody()
        : partitions_()
        , failures_()
        , firstNonProcessedRequestIndex_(0)
    {
    }

    ServiceLocationNotificationReplyBody::ServiceLocationNotificationReplyBody(
        std::vector<ResolvedServicePartition> && partitions,
        std::vector<AddressDetectionFailure> && failures,
        uint64 firstNonProcessedRequestIndex)
        : partitions_(std::move(partitions))
        , failures_(std::move(failures))
        , firstNonProcessedRequestIndex_(firstNonProcessedRequestIndex)
    {
    }
}
