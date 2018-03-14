// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Indicates if the transaction is active or not
    //
    namespace TransactionState
    {
        enum Enum
        {
            Active = 0,

            // A read is ongoing
            Reading = 1,

            // Transaction is being committed
            Committing = 2,

            // Transaction is being aborted
            Aborting = 3,

            // Transaction is committed
            Committed = 4,

            // Transaction is aborted
            Aborted = 5,

            // Transaction failed to commit/abort due to an exception
            Faulted = 6,

            LastValidEnum = Faulted
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        DECLARE_ENUM_STRUCTURED_TRACE(TransactionState);
    }
}
