// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace EndpointType
    {
        enum Enum
        {
            Internal = 0,
            Input = 1,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        EndpointType::Enum GetEndpointType(std::wstring const & val);

		std::wstring EnumToString(EndpointType::Enum value);
    };
}
