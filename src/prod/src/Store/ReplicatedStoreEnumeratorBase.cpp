// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

ErrorCode ReplicatedStoreEnumeratorBase::TryMoveNextBase(bool & success)
{
    auto error = this->OnMoveNextBase();

    if (error.IsError(ErrorCodeValue::EnumerationCompleted))
    {
        success = false;

        error = ErrorCodeValue::Success;
    }
    else
    {
        success = error.IsSuccess();
    }

    return error;
}
