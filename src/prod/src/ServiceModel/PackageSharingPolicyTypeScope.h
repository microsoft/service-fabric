// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace PackageSharingPolicyTypeScope
    {
        enum Enum
        {
            None = 0x0000,
            All = 0x0001,
            Code = 0x0002,
            Config = 0x0003,
            Data = 0x0004
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        Common::ErrorCode FromString(std::wstring const & value, __out Enum & val);
		std::wstring EnumToString(PackageSharingPolicyTypeScope::Enum val);
    };
}
