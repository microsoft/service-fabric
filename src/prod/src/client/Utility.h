// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class Utility
    {
        DENY_COPY(Utility);

    public:
        static size_t GetMessageContentThreshold();

        static Common::ErrorCode CheckEstimatedSize(size_t estimatedSize);
        static Common::ErrorCode CheckEstimatedSize(size_t estimatedSize, size_t maxAllowedSize);
    };
}

