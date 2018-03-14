// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace SecondaryNotificationMode
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Invalid: w << "Invalid"; return;
                case None: w << "None"; return;
                case NonBlockingQuorumAcked: w << "NonBlockingQuorumAcked"; return;
                case BlockSecondaryAck: w << "BlockSecondaryAck"; return;
            }
            w << "SecondaryNotificationMode(" << static_cast<uint>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( SecondaryNotificationMode, FirstValidEnum, LastValidEnum )

        Enum FromPublicApi(FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE mode)
        {
            switch (mode)
            {
                case FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_NONE:
                case FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_INVALID:
                    return None;

                case FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_NON_BLOCKING_QUORUM_ACKED:
                    return NonBlockingQuorumAcked;

                case FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_BLOCK_SECONDARY_ACK:
                    return BlockSecondaryAck;

                default:
                    return Invalid;
            }
        }

        FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE ToPublicApi(Enum mode)
        {
            switch (mode)
            {
                case None:
                    return FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_NONE;

                case NonBlockingQuorumAcked:
                    return FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_NON_BLOCKING_QUORUM_ACKED;

                case BlockSecondaryAck:
                    return FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_BLOCK_SECONDARY_ACK;

                default:
                    return FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_INVALID;
            }
        }
    }
}
