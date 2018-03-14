// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStoreEnumeratorBase : public Common::ComponentRoot
    {
        DENY_COPY(ReplicatedStoreEnumeratorBase)

    protected:

        ReplicatedStoreEnumeratorBase() { }

        virtual ~ReplicatedStoreEnumeratorBase() { }

        virtual Common::ErrorCode OnMoveNextBase() = 0;

        Common::ErrorCode TryMoveNextBase(bool & success);
    };
}
