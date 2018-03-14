// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class RecoveryInformation 
        : KObject<RecoveryInformation>
    {
        
    public:

        RecoveryInformation();

        RecoveryInformation(__in bool shouldRemoveLocalStateDueToIncompleteRestore) noexcept;

        // Note: F.6: Noexcept is used to ensure FailFast brings the process down.
        RecoveryInformation(
            __in bool shouldSkipRecoveryDueToIncompleteChangeRoleNone,
            __in bool shouldRemoveLocalStateDueToIncompleteRestore) noexcept;

        RecoveryInformation(__in RecoveryInformation const & other);

        void operator=(__in RecoveryInformation const & other);

        __declspec(property(get = get_LogCompleteCheckpointAfterRecovery)) bool LogCompleteCheckpointAfterRecovery;
        bool get_LogCompleteCheckpointAfterRecovery() const 
        { 
            return logCompleteCheckpointAfterRecovery_; 
        }

        __declspec(property(get = get_ShouldRemoveLocalStateDueToIncompleteRestore)) bool ShouldRemoveLocalStateDueToIncompleteRestore;
        bool get_ShouldRemoveLocalStateDueToIncompleteRestore() const 
        { 
            return shouldRemoveLocalStateDueToIncompleteRestore_; 
        }

        __declspec(property(get = get_ShouldSkipRecoveryDueToIncompleteChangeRoleNone)) bool ShouldSkipRecoveryDueToIncompleteChangeRoleNone;
        bool get_ShouldSkipRecoveryDueToIncompleteChangeRoleNone() const 
        { 
            return shouldSkipRecoveryDueToIncompleteChangeRoleNone_; 
        }

    private:

        bool logCompleteCheckpointAfterRecovery_;
        bool shouldRemoveLocalStateDueToIncompleteRestore_;
        bool shouldSkipRecoveryDueToIncompleteChangeRoleNone_;
    };
}
