// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;


FABRIC_PARTITION_ID ServiceInfoUtility::GetPartitionId(
	const FABRIC_SERVICE_PARTITION_INFORMATION * servicePartitionInfo)
{
	switch (servicePartitionInfo->Kind)
	{
		case FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
			return ((FABRIC_INT64_RANGE_PARTITION_INFORMATION *)servicePartitionInfo->Value)->Id;
            break;

        case FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_NAMED:
			return ((FABRIC_NAMED_PARTITION_INFORMATION *)servicePartitionInfo->Value)->Id;
            break;

        case FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_SINGLETON:
			return ((FABRIC_SINGLETON_PARTITION_INFORMATION *)servicePartitionInfo->Value)->Id;
            break;

        default:
            Assert::CodingError("Unknown FABRIC_SERVICE_PARTITION_KIND {0}", (LONG)(servicePartitionInfo->Kind));
	}
}
