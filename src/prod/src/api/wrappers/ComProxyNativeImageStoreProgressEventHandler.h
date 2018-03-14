// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyNativeImageStoreProgressEventHandler :
        public Common::ComponentRoot,
        public INativeImageStoreProgressEventHandler
    {
        DENY_COPY(ComProxyNativeImageStoreProgressEventHandler);

    public:
        explicit ComProxyNativeImageStoreProgressEventHandler(Common::ComPointer<IFabricNativeImageStoreProgressEventHandler> const & comImpl);

        virtual ~ComProxyNativeImageStoreProgressEventHandler();

        //
        // INativeImageStoreProgressEventHandler
        //
        Common::ErrorCode GetUpdateInterval(__out Common::TimeSpan &) override;

        Common::ErrorCode OnUpdateProgress(size_t completedItems, size_t totalItems, ServiceModel::ProgressUnitType::Enum itemType) override;

    private:
        Common::ComPointer<IFabricNativeImageStoreProgressEventHandler> const comImpl_;
    };
}
