// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

namespace Hosting2
{
    namespace ManageOverlayNetworkAction
    {
        void ManageOverlayNetworkAction::WriteToTextWriter(Common::TextWriter & w, Enum const & val)
        {
            w << ToString(val);
        }

        std::wstring ManageOverlayNetworkAction::ToString(Enum const & val)
        {
            switch (val)
            {
            case Assign:
                return L"Assign";
            case Unassign:
                return L"Unassign";
            default:
                return wformatString("{0:x}", (unsigned int)val);
            }
        }
    }
}
