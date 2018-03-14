// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability
{
    namespace ReplicationComponent
    {
        ComReplicator::BaseOperation::BaseOperation(Replicator & replicator)
            : Common::ComAsyncOperationContext(true),
              replicator_(replicator)
        {
        }

        ComReplicator::BaseOperation::~BaseOperation()
        {
        }

        HRESULT STDMETHODCALLTYPE ComReplicator::BaseOperation::Initialize(
            Common::ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback)
        {
            HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
            if (FAILED(hr)) { return hr; }
            return hr;
        }

    } // end namespace ReplicationComponent
} // end namespace Reliability
