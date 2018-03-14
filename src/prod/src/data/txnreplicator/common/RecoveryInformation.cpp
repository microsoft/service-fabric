// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;

RecoveryInformation::RecoveryInformation()
    : KObject()
    , logCompleteCheckpointAfterRecovery_(false)
    , shouldRemoveLocalStateDueToIncompleteRestore_(false)
    , shouldSkipRecoveryDueToIncompleteChangeRoleNone_(false)
{
}

RecoveryInformation::RecoveryInformation(__in bool shouldRemoveLocalStateDueToIncompleteRestore) noexcept
    : KObject()
    , logCompleteCheckpointAfterRecovery_(false)
    , shouldRemoveLocalStateDueToIncompleteRestore_(shouldRemoveLocalStateDueToIncompleteRestore)
    , shouldSkipRecoveryDueToIncompleteChangeRoleNone_(true)
{
}

RecoveryInformation::RecoveryInformation(
    __in bool logCompleteCheckpointAfterRecovery,
    __in bool shouldRemoveLocalStateDueToIncompleteRestore) noexcept
    : KObject()
    , logCompleteCheckpointAfterRecovery_(logCompleteCheckpointAfterRecovery)
    , shouldRemoveLocalStateDueToIncompleteRestore_(shouldRemoveLocalStateDueToIncompleteRestore)
    , shouldSkipRecoveryDueToIncompleteChangeRoleNone_(false)
{
    ASSERT_IFNOT(
        !shouldSkipRecoveryDueToIncompleteChangeRoleNone_,
        "RecoveryInformation is instantiated with shouldSkipRecoveryDueToIncompleteChangeRoleNone_ = true");
}

RecoveryInformation::RecoveryInformation(__in RecoveryInformation const & other)
{
    if (this != &other)
    {
        logCompleteCheckpointAfterRecovery_ = other.logCompleteCheckpointAfterRecovery_;
        shouldRemoveLocalStateDueToIncompleteRestore_ = other.shouldRemoveLocalStateDueToIncompleteRestore_;
        shouldSkipRecoveryDueToIncompleteChangeRoleNone_ = other.shouldSkipRecoveryDueToIncompleteChangeRoleNone_;
    }
}

void RecoveryInformation::operator=(__in RecoveryInformation const & other)
{
    if (this != &other)
    {
        logCompleteCheckpointAfterRecovery_ = other.logCompleteCheckpointAfterRecovery_;
        shouldRemoveLocalStateDueToIncompleteRestore_ = other.shouldRemoveLocalStateDueToIncompleteRestore_;
        shouldSkipRecoveryDueToIncompleteChangeRoleNone_ = other.shouldSkipRecoveryDueToIncompleteChangeRoleNone_;
    }
}
