// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace KtlLogger
{
    class Constants
    {
    public:
        static const std::wstring DefaultApplicationSharedLogIdString;
        static const Common::Guid DefaultApplicationSharedLogId;
        static const std::wstring DefaultApplicationSharedLogName;
        static const std::wstring DefaultApplicationSharedLogSubdirectory;
        static const std::wstring DefaultSystemServicesSharedLogIdString;
        static const Common::Guid DefaultSystemServicesSharedLogId;
        static const std::wstring DefaultSystemServicesSharedLogName;
        static const std::wstring DefaultSystemServicesSharedLogSubdirectory;
        static const std::wstring NullGuidString;
    };
}
