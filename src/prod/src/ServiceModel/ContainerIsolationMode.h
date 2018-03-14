// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ContainerIsolationMode
    {
        enum Enum
        {
            process = 0,
            hyperv = 1
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        Common::ErrorCode FromString(std::wstring const & value, __out Enum & val);
        std::wstring EnumToString(ServiceModel::ContainerIsolationMode::Enum val);

        ::FABRIC_CONTAINER_ISOLATION_MODE ToPublicApi(__in Enum);
    };
}
