// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ContainerHelper
    {
        DENY_COPY(ContainerHelper)

    public:
        static ContainerHelper & GetContainerHelper();
        ~ContainerHelper();
        Common::ErrorCode GetCurrentOsBuildNumber(__out std::wstring & os);
        Common::ErrorCode GetContainerImageName(ServiceModel::ImageOverridesDescription const& images, __inout std::wstring & imageName);

    private:
        ContainerHelper();

    private:
        std::wstring osBuildNumber_;
        bool isInitialized_;
        Common::RwLock lock_;

        static ContainerHelper * singleton_;
        static INIT_ONCE initOnceContainerHelper_;
        static BOOL CALLBACK InitContainerHelper(PINIT_ONCE, PVOID, PVOID *);
    };
}
