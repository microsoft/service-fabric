// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace PersistenceState
    {
        enum Enum
        {
            NoChange = 0,
            ToBeUpdated = 1,
            ToBeInserted = 2,
            ToBeDeleted = 3,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }
}
