// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // A base class for the lock context.
    // 
    class LockContext 
        : public OperationContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(LockContext)

    public:
        // Gets or sets a value indicating tracking context.
        bool IsTrackingContext();

        // Port Note: Since lock manager is not used, constructor with lock manager is not required.
        // Port Note: Empty constructor not required.
        
        // Releases locks.
        // Port Note: Since lock manager is not used, no reason to have virtual implementation.
        virtual void Unlock() = 0;

        // Port Note: Both overloads of IsEqual is never used.
    private:
        // Port Note: lockManager is never used.

        // Indicates if the context is being tracked or not.
        bool isTracking_;
    };
}
