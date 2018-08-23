// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class LockManager;
        class ILock : public KObject<ILock>, public KShared<ILock>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(ILock)
        public:
            virtual LONG64 GetOwner() const = 0;
            virtual LockMode::Enum GetLockMode() const = 0;
            virtual Common::TimeSpan GetTimeOut() const = 0;
            virtual LockStatus::Enum GetStatus() const = 0;
            virtual LONG64 GetGrantedTime() const = 0;
            virtual ULONG32 GetCount() const = 0;
            virtual bool IsUgraded() const = 0;
            virtual KSharedPtr<LockManager> GetLockManager() const = 0;
        };
    }
}
