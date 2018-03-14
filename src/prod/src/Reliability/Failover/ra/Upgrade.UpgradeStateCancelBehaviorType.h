// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace UpgradeStateCancelBehaviorType
            {
                enum Enum
                {
                    // An upgrade state where Cancel is not allowed but the rollback can be performed immediately
                    // Example: ApplicationUpgrade_WaitForCloseCompletionCheck where RA cannot process the cancel message
                    // as replicas may go down at any point of time but a rollback can be started immediately
                    NonCancellableWithImmediateRollback = 0,

                    // An upgrade state where Cancel is not allowed and the rollback can be performed only after completing
                    // execution of the current state
                    // Example: ApplicationUpgrade_UpdateVersionsAndClose where RA cannot process the cancel message
                    // as replicas may go down at any point of time after
                    // and the rollback cannot be processed as the state of FT is being updated 
                    NonCancellableWithDeferredRollback = 1,

                    // An upgrade state where cancel is not allowed and rollback cannot be performed
                    // This would require Rollback to be retried by the sender
                    // An example of this is fabric upgrade where replicas are being closed
                    NonCancellableWithNoRollback = 2,


                    // An upgrade state where Cancel is allowed and the rollback can be performed immediately
                    // Example: ApplicationUpgrade_Download:
                    //  cancel can allow the current async download op to complete in a separate thread and send the reply
                    //  Rollback can immediately begin and allow the current download to complete separately
                    CancellableWithImmediateRollback = 3,
                };
            }
        }
    }
}
