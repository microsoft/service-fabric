// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    namespace HealthReportClientState
    {
        enum Enum : int
        {
            PendingSend = 0,
            PendingAck = 1,

            LAST_STATE = PendingAck
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(HealthReportClientState);
    };
}
