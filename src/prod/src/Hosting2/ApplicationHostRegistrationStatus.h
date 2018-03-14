// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace ApplicationHostRegistrationStatus
    {
        enum Enum
        {
            NotRegistered = 0,
            InProgress = 1,
            InProgress_Monitored = 2,
            Completed = 3,
            Timedout = 4,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        std::wstring ToString(Enum const & val);
    }
}
