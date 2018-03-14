// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace ReplicatedStoreState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Created: w << "Created"; return;
                case Opened: w << "Opened"; return;
                case PrimaryPassive: w << "PrimaryPassive"; return;
                case PrimaryActive: w << "PrimaryActive"; return;
                case PrimaryChangePending: w << "PrimaryChangePending"; return;
                case PrimaryClosePending: w << "PrimaryClosePending"; return;
                case SecondaryPassive: w << "SecondaryPassive"; return;
                case SecondaryActive: w << "SecondaryActive"; return;
                case SecondaryChangePending: w << "SecondaryChangePending"; return;
                case SecondaryClosePending: w << "SecondaryClosePending"; return;
                case Closed: w << "Closed"; return;
            }
            w << "ReplicatedStoreState(" << static_cast<uint>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( ReplicatedStoreState, FirstValidEnum, LastValidEnum )
    }
}
