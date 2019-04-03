// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Naming
{
    class ServiceResolutionParameters
    {
    public:
        ServiceResolutionParameters()
        {
        }

        ServiceResolutionParameters(
            Reliability::CacheMode::Enum cacheMode,
            int64 previousResolveLocationVersion,
            int64 previousServiceDescriptorVersion)
          : cacheMode_(cacheMode)
          , previousResolveLocationVersion_(previousResolveLocationVersion)
          , previousServiceDescriptorVersion_(previousServiceDescriptorVersion)
        {
        }

        __declspec(property(get=get_CacheMode)) Reliability::CacheMode::Enum CacheMode;
        __declspec(property(get=get_PreviousResolveLocationVersion, put=put_PreviousResolveLocationVersion)) int64 PreviousResolveLocationVersion;
        __declspec(property(get=get_PreviousServiceDescriptorVersion, put=put_PreviousServiceDescriptorVersion)) int64 PreviousServiceDescriptorVersion;

        Reliability::CacheMode::Enum get_CacheMode() const { return cacheMode_; }
        int64 get_PreviousResolveLocationVersion() const { return previousResolveLocationVersion_; }
        void put_PreviousResolveLocationVersion(int64 value) { previousResolveLocationVersion_ = value; }
        int64 get_PreviousServiceDescriptorVersion() const { return previousServiceDescriptorVersion_; }
        void put_PreviousServiceDescriptorVersion(int64 value) { previousServiceDescriptorVersion_ = value; }

    private:
        Reliability::CacheMode::Enum cacheMode_;
        int64 previousResolveLocationVersion_;
        int64 previousServiceDescriptorVersion_;
    };
}
