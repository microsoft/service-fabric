// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace CodePackageIsolationPolicyType
    {
        enum Enum
        {
            Invalid = 0,
            SharedDomain = 1,
            DedicatedDomain = 2,
            DedicatedProcess = 3,
            Container = 4
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        std::wstring ToString(Enum const & val);
        Common::ErrorCode FromPublicApi(FABRIC_DLLHOST_ISOLATION_POLICY const & publicVal, __out Enum & val);
        FABRIC_DLLHOST_ISOLATION_POLICY ToPublicApi(Enum const & val);

        bool TryParseFromString(std::wstring const& string, __out Enum & val);
    };
}
