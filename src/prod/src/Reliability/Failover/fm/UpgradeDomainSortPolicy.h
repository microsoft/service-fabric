// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace UpgradeDomainSortPolicy
        {
            enum Enum
            {
                Lexicographical = 0,
                DigitsAsNumbers = 1,
                LastValidEnum = DigitsAsNumbers
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

            DECLARE_ENUM_STRUCTURED_TRACE(UpgradeDomainSortPolicy);
        };
    }
}
