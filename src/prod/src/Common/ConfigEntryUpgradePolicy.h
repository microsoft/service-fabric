// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    namespace ConfigEntryUpgradePolicy
    {
        enum Enum
        {
            Dynamic = 0,
            Static = 1,
            NotAllowed = 2,
            SingleChange = 3,
        };

        void WriteToTextWriter(TextWriter & w, Enum const & val);
        std::wstring ToString(Enum const & val);        
        Enum FromString(std::wstring const & val);        
    }
}
