// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        // Used for controlling access to producer-consumer buffers in PLB:
        //    - It can act as a regular exclusive lock when locked with LockAndSetBooleanLock.
        //    - It contains a boolean variable that can be updated only when locking.
        //    - Threads can check if boolean variable is set before attempting to take the lock.
        class BooleanLock
        {
            friend class LockAndSetBooleanLock;
        public:
            BooleanLock(bool initialValue) : variable_(initialValue) {}

            bool IsSet() { return variable_.load(); }
        private:
            Common::ExclusiveLock lock_;
            Common::atomic_bool variable_;
        };

        // Takes the lock on BooleanLock variable and sets the bool value to value that is provided.
        // Default for new value is true, because updating buffers is a common case.
        // Lock is released once the object is destroyed.
        class LockAndSetBooleanLock
        {
        public:
            LockAndSetBooleanLock(BooleanLock & variable, bool value = true)
                : grab_(variable.lock_)
            {
                variable.variable_.store(value);
            }
        private:
            Common::AcquireExclusiveLock grab_;
        };

    }
}

