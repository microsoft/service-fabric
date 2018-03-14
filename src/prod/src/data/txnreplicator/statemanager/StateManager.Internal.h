// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// State Manager.
#include "Constants.h"

#include "ITransactionStateManager.h"
#include "IStateManager.h"

// Structs
#include "StateManagerFactory.h"

inline bool NEXT_EXIST(__in NTSTATUS status) noexcept
{
    ASSERT_IFNOT(
        NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES,
        "StateManager: NEXT_EXIST should be either success or no more entries. Status: {0}",
        status);

    return status == STATUS_SUCCESS ? true : false;
}

