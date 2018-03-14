// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace ReplicatedStoreState
    {
        enum Enum
        {
            Created = 0,
            Opened = 1,
            PrimaryPassive = 2,
            PrimaryActive = 3,
            PrimaryChangePending = 4,
            PrimaryClosePending = 5,
            SecondaryPassive = 6,
            SecondaryActive = 7,
            SecondaryChangePending = 8,
            SecondaryClosePending = 9,
            Closed = 10,

            FirstValidEnum = Created,
            LastValidEnum = Closed
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( ReplicatedStoreState )
    };
}
