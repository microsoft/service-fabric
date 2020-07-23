// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace SecurityPrincipalAccountType
    {
        enum Enum
        {
            LocalUser = 0,
            DomainUser = 1,
            NetworkService = 2,
            LocalService = 3,
            ManagedServiceAccount = 4,
            LocalSystem = 5,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        Common::ErrorCode FromString(std::wstring const & value, __out Enum & val);
        std::wstring ToString(ServiceModel::SecurityPrincipalAccountType::Enum val);
        void Validate(ServiceModel::SecurityPrincipalAccountType::Enum val);
    };
}
