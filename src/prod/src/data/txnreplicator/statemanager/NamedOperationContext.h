// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // NamedOperationContext allows state provider Id to be piggy backed in 
        // the user's (or SM's) operation context.
        //
        class NamedOperationContext final :
            public TxnReplicator::OperationContext
        {
            K_FORCE_SHARED(NamedOperationContext)

        public: // Static creator
            static NTSTATUS Create(
                __in OperationContext const * operationContext,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;
        
        public: // Properties.
            __declspec(property(get = get_OperationContext)) OperationContext::CSPtr OperationContextCSPtr;
            OperationContext::CSPtr NamedOperationContext::get_OperationContext() const;

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID NamedOperationContext::get_StateProviderId() const;

        private: // Constructor
            NOFAIL NamedOperationContext(
                __in OperationContext const * operationContext,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId);

        private:
            OperationContext::CSPtr operationContextCSPtr_;
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
        };
    }
}
