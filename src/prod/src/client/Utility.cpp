// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;
using namespace ServiceModel;

namespace Client
{
    size_t Utility::GetMessageContentThreshold()
    {
        return static_cast<size_t>(
            static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) * ServiceModelConfig::GetConfig().MessageContentBufferRatio);
    }

    ErrorCode Utility::CheckEstimatedSize(size_t estimatedSize)
    {
        return CheckEstimatedSize(estimatedSize, GetMessageContentThreshold());
    }

    ErrorCode Utility::CheckEstimatedSize(
        size_t estimatedSize,
        size_t maxAllowedSize)
    {
        // The entry must be less than the max allowed size.
        if (estimatedSize >= maxAllowedSize)
        {
            Trace.WriteInfo(
                Constants::FabricClientSource,
                "Entry size {0} is larger than allowed size {1}", 
                estimatedSize, 
                maxAllowedSize);

            return ErrorCode(ErrorCodeValue::EntryTooLarge);
        }

        return ErrorCode(ErrorCodeValue::Success);
    }
}
