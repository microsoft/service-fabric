// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;

namespace Data
{
    namespace LoggingReplicator
    {
        namespace PeriodicCheckpointTruncationState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case NotStarted:
                    w << L"NotStarted"; return;
                case Ready:
                    w << L"Ready"; return;
                case CheckpointStarted:
                    w << L"CheckpointStarted"; return;
                case CheckpointCompleted:
                    w << L"CheckpointCompleted"; return;
                case TruncationStarted:
                    w << L"TruncationStarted"; return;
                default:
                    CODING_ASSERT("Unknown TruncationState:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(PeriodicCheckpointTruncationState, NotStarted, LastValidEnum)
        }
    }
}
