// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;

namespace TxnReplicator
{
    namespace TransactionState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Active:
                w << L"Active"; return;
            case Reading:
                w << L"Reading"; return;
            case Committing:
                w << L"Committing"; return;
            case Aborting:
                w << L"Aborting"; return;
            case Committed:
                w << L"Committed"; return;
            case Aborted:
                w << L"Aborted"; return;
            case Faulted:
                w << L"Faulted"; return;
            default:
                Common::Assert::CodingError("Unknown TransactionState:{0} in Replicator", val);
            }
        }

        ENUM_STRUCTURED_TRACE(TransactionState, Active, LastValidEnum)
    }
}
