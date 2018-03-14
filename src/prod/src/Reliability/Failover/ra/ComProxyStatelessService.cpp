// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

AsyncOperationSPtr ComProxyStatelessService::BeginOpen(ComPointer<::IFabricStatelessServicePartition> const & partition, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(partition, "Partition cannot be null.");

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(partition, *this, callback, parent);
}

ErrorCode ComProxyStatelessService::EndOpen(AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return OpenAsyncOperation::End(asyncOperation, serviceLocation);
}

AsyncOperationSPtr ComProxyStatelessService::BeginClose(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode ComProxyStatelessService::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return CloseAsyncOperation::End(asyncOperation);
}

void ComProxyStatelessService::Abort()
{
    service_->Abort();
}
