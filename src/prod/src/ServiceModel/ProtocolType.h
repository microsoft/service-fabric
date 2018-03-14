// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ProtocolType
    {
        enum Enum
        {
            Tcp = 0,
            Http = 1,
            Https = 2,
            Udp = 3
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        ProtocolType::Enum GetProtocolType(std::wstring const & val);

		std::wstring EnumToString(ProtocolType::Enum value);
    };
}
