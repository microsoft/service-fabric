// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComReplicator::BaseOperation : 
            public Common::ComAsyncOperationContext
        {
            DENY_COPY(BaseOperation)

        public:
            virtual ~BaseOperation();

        protected:
            explicit BaseOperation(Replicator & replicator);

            __declspec(property(get=get_Engine)) Replicator & ReplicatorEngine;
            Replicator & get_Engine() { return this->replicator_; }

            HRESULT STDMETHODCALLTYPE Initialize(
                Common::ComponentRootSPtr const & rootSPtr,
                __in_opt IFabricAsyncOperationCallback * callback);

        private:
            Replicator & replicator_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
