// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        namespace CopyStage
        {
            //
            // The copy stage.
            //
            enum Enum
            {
                // The invalid.
                Invalid = 0,

                // The copy metadata.
                CopyMetadata = 1,

                // The copy none.
                CopyNone = 2,

                // The copy state.
                CopyState = 3,

                // The copy progress vector.
                CopyProgressVector = 4,

                // The false progress.
                CopyFalseProgress = 5,

                // The scan to starting lsn.
                CopyScanToStartingLsn = 6,

                // The copy log.
                CopyLog = 7,

                // The copy done.
                CopyDone = 8,

                LastValidEnum = CopyDone
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(CopyStage);
        }
    }
}
