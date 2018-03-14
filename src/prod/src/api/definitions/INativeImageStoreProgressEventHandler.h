// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class INativeImageStoreProgressEventHandler
    {
    public:
        virtual ~INativeImageStoreProgressEventHandler() {};

        virtual Common::ErrorCode GetUpdateInterval(__out Common::TimeSpan &) = 0;

        virtual Common::ErrorCode OnUpdateProgress(size_t completedItems, size_t totalItems, ServiceModel::ProgressUnitType::Enum itemType) = 0;
    };
}
