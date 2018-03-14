// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace GrantAccessType
    {
        enum Enum
        {
            Read = 0,
            Change = 1,
            Full = 2,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

        GrantAccessType::Enum GetGrantAccessType(std::wstring const & val);
		std::wstring ToString(ServiceModel::GrantAccessType::Enum);
    };
}
