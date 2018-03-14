// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Store
{
    ErrorCode ReplicatedStore::ChangeRoleAsyncOperation::End(AsyncOperationSPtr const & operation, __out ::FABRIC_REPLICA_ROLE & newRole)
    {
        auto casted = AsyncOperation::End<ChangeRoleAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            newRole = casted->newRole_;
        }

        return casted->Error;
    }
}
