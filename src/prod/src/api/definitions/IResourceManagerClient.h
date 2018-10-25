// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IResourceManagerClient
    {
    public:
        virtual ~IResourceManagerClient() {};

        virtual Common::AsyncOperationSPtr BeginClaimResource(
            Management::ResourceManager::Claim const & claim,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndClaimResource(
            Common::AsyncOperationSPtr const & operation,
            __out Management::ResourceManager::ResourceMetadata & metadata) = 0;

        virtual Common::AsyncOperationSPtr BeginReleaseResource(
            Management::ResourceManager::Claim const & claim,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndReleaseResource(
            Common::AsyncOperationSPtr const & operation) = 0;
    };
}
