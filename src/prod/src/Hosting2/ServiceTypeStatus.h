// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace ServiceTypeStatus
    {
        enum Enum
        {
            Invalid                         = 0,
            NotRegistered_Enabled           = 1,
            InProgress_Enabled              = 2,
            Registered_Enabled              = 4,
            NotRegistered_DisableScheduled  = 8,
            NotRegistered_Disabled          = 16,
            InProgress_Disabled             = 32,
            Closed                          = 64,

            Registered_Mask                 = 4,
            Disabled_Mask                   = 48,
            DisableScheduled_Mask           = 8,
            InProgress_Mark                 = 34
        };

        inline bool IsRegistered(Enum const & val) { return ((val & Registered_Mask) != 0); }
        inline bool IsInProgress(Enum const & val) { return ((val & InProgress_Mark) != 0); }
        inline bool IsDisabled(Enum const & val) { return ((val & Disabled_Mask) != 0); }
        inline bool IsDisableScheduled(Enum const & val) { return ((val & DisableScheduled_Mask) != 0); }

        FABRIC_SERVICE_TYPE_REGISTRATION_STATUS ToPublicRegistrationStatus(ServiceTypeStatus::Enum serviceTypeStatus);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        std::wstring ToString(Enum const & val);
    }
}
