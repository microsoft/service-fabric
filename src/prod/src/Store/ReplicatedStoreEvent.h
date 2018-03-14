// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace ReplicatedStoreEvent
    {
        enum Enum
        {
            Invalid = 0,
            Open = 1,
            ChangePrimary = 2,
            ChangeSecondary = 3,
            StartTransaction = 4,
            FinishTransaction = 5,
            SecondaryPumpClosed = 6,
            Close = 7,

            FirstValidEnum = Open,
            LastValidEnum = Close
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( ReplicatedStoreEvent )
    };
}
