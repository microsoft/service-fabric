// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ApplicationIsolationPolicyType
    {
        enum Enum
        {
            Invalid = 0,
            SharedProcess = 1,
            DedicatedProcess = 2,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        Common::ErrorCode FromString(std::wstring const & value, __out Enum & val);
    };
}
