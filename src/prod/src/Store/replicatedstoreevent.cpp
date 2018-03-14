// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace ReplicatedStoreEvent
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Invalid: w << "Invalid"; return;
                case Open: w << "Open"; return;
                case ChangePrimary: w << "ChangePrimary"; return;
                case ChangeSecondary: w << "ChangeSecondary"; return;
                case StartTransaction: w << "StartTransaction"; return;
                case FinishTransaction: w << "FinishTransaction"; return;
                case SecondaryPumpClosed: w << "SecondaryPumpClosed"; return;
                case Close: w << "Close"; return;
            }
            w << "ReplicatedStoreEvent(" << static_cast<uint>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( ReplicatedStoreEvent, FirstValidEnum, LastValidEnum )
    }
}
