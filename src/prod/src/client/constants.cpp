// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    // -------------------------------
    // Tracing
    // -------------------------------
    Common::StringLiteral const Constants::FabricClientSource("FabricClient");

    Common::StringLiteral const Constants::HealthClientReportSource("Report");
    Common::StringLiteral const Constants::HealthClientSequenceStreamSource("SS");

    LONGLONG Constants::CurrentNotificationHandlerId(0L);

}
