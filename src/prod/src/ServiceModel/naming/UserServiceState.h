// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace UserServiceState
    {
        enum Enum
        {
            Invalid = 0,
            None = 1,
            Creating = 2,
            Deleting = 3,
            Created = 4,
            Updating = 5,
            ForceDeleting = 6,
        };

        bool IsDeleting(Enum const & value);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
