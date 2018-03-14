// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Client
{
    namespace HealthReportClientState
    {
        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case PendingSend: w << "PendingSend"; return;
            case PendingAck: w << "PendingAck"; return;
            default: w << "HealthReportClientState(" << static_cast<uint>(e) << ')'; return;
            }
        }

        ENUM_STRUCTURED_TRACE(HealthReportClientState, PendingSend, LAST_STATE);
    }
}
