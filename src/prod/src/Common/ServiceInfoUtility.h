// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ServiceInfoUtility
    {
    public:
        static FABRIC_PARTITION_ID GetPartitionId(
			const FABRIC_SERVICE_PARTITION_INFORMATION * servicePartitionInfo);
    };
}
