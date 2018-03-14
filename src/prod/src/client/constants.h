// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class Constants
    {
    public:

        //
        // Tracing
        //
        static Common::StringLiteral const FabricClientSource;

        static Common::StringLiteral const HealthClientReportSource;
        static Common::StringLiteral const HealthClientSequenceStreamSource;

        static LONGLONG CurrentNotificationHandlerId;
    };
}

