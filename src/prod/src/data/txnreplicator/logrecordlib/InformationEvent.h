// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        namespace InformationEvent
        {
            enum Enum
            {
                Invalid = 0,

                Recovered = 1,

                CopyFinished = 2,

                ReplicationFinished = 3,

                Closed = 4,

                PrimarySwap = 5,

                RestoredFromBackup = 6,

                // This indicates that change role to none was called and no new records are inserted after this
                RemovingState = 7,

                LastValidEnum = RemovingState
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(CopyStage);
        }
    }
}
