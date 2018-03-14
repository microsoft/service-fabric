// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace ApplicationHostType
    {
        enum Enum
        {
            NonActivated = 0,
            Activated_SingleCodePackage = 1,
            Activated_MultiCodePackage = 2,
            Activated_InProcess = 3
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        std::wstring ToString(Enum const & val);
        Common::ErrorCode FromString(std::wstring const & str, __out Enum & val);
    }
}
