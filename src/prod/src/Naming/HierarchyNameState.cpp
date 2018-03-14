// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    namespace HierarchyNameState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << "Invalid"; return;
            case Creating: w << "Creating"; return;
            case Deleting: w << "Deleting"; return;
            case Created: w << "Created"; return;
            }

            w << "HierarchNameState(" << static_cast<int>(e) << ')';
        }
    }
}
