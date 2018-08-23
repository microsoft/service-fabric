// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class PrimeLockRequest : public KObject<PrimeLockRequest>, public KShared<PrimeLockRequest>
        {
            K_FORCE_SHARED(PrimeLockRequest)

        public:
            static NTSTATUS 
                Create(
                    __in LockManager& lockManager, 
                    __in LockMode::Enum lockMode, 
                    __in KAllocator& allocator, 
                    __out PrimeLockRequest::SPtr& result);
            
            __declspec(property(get = get_LockManagerSPtr)) KSharedPtr<LockManager> LockManagerSPtr;
            KSharedPtr<LockManager> get_LockManagerSPtr() const
            {
                return lockManagerSPtr_;
            }

            __declspec(property(get = get_Mode)) LockMode::Enum Mode;
            LockMode::Enum get_Mode() const
            {
                return lockMode_;
            }

        private:
            PrimeLockRequest(__in LockManager& lockManager, __in LockMode::Enum lockMode);

            KSharedPtr<LockManager> lockManagerSPtr_;
            LockMode::Enum lockMode_;
        };
    }
}

