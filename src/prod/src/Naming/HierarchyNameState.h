// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    namespace HierarchyNameState
    {
        enum Enum
        {
            Invalid = 0,
            Creating = 1,
            Deleting = 2,
            Created = 3,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
