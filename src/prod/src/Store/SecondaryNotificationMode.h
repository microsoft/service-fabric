// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace SecondaryNotificationMode
    {
        enum Enum
        {
            Invalid = 0,
            None = 1,
            NonBlockingQuorumAcked = 2,
            BlockSecondaryAck = 3,

            FirstValidEnum = Invalid,
            LastValidEnum = BlockSecondaryAck,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( SecondaryNotificationMode )

        Enum FromPublicApi(FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE);
        FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE ToPublicApi(Enum);
    };
}

