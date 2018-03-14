// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    namespace UserServiceState
    {
        bool IsDeleting(Enum const & value)
        {
            return value == Enum::Deleting || value == Enum::ForceDeleting;
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid: w << "Invalid"; return;
            case None: w << "None"; return;
            case Creating: w << "Creating"; return;
            case Deleting: w << "Deleting"; return;
            case Created: w << "Created"; return;
            case Updating: w << "Updating"; return;
            case ForceDeleting: w << "ForceDeleting"; return;
            }

            w << "UserServiceStatus(" << static_cast<int>(e) << ')';
        }
    }
}
