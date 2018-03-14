// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Reliability::ReconfigurationAgentComponent;

StringLiteral const TraceComponent("ComProxyStatefulService");

ComProxyStatefulService::ComProxyStatefulService()
    : service_()
    , internalService_()
{
    // Empty
}

ComProxyStatefulService::ComProxyStatefulService(Common::ComPointer<::IFabricStatefulServiceReplica> const & service)
    : service_(service)
    , internalService_()
{
    auto hr = service_->QueryInterface(IID_IFabricInternalStatefulServiceReplica, internalService_.VoidInitializationAddress());

    if (SUCCEEDED(hr))
    {
        Trace.WriteNoise(TraceComponent, "IFabricInternalStatefulServiceReplica supported");
    }
    else
    {
        if (hr == E_NOINTERFACE)
        {
            Trace.WriteNoise(TraceComponent, "IFabricInternalStatefulServiceReplica not supported");
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "Failed to QI for IFabricInternalStatefulServiceReplica: hr={0}", hr);
        }
    }
}

AsyncOperationSPtr ComProxyStatefulService::BeginOpen(
    ::FABRIC_REPLICA_OPEN_MODE openMode,
    ComPointer<::IFabricStatefulServicePartition> const & partition, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(partition, "Partition cannot be null.");

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(openMode, partition, *this, callback, parent);
}

ErrorCode ComProxyStatefulService::EndOpen(AsyncOperationSPtr const & asyncOperation, __out ComProxyReplicatorUPtr & replication)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return OpenAsyncOperation::End(asyncOperation, replication);
}

AsyncOperationSPtr ComProxyStatefulService::BeginChangeRole(::FABRIC_REPLICA_ROLE newRole, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(newRole, *this, callback, parent);
}

ErrorCode ComProxyStatefulService::EndChangeRole(AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return ChangeRoleAsyncOperation::End(asyncOperation, serviceLocation);
}

AsyncOperationSPtr ComProxyStatefulService::BeginClose(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode ComProxyStatefulService::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    ASSERT_IFNOT(asyncOperation, "asyncOperation cannot be null.");
    return CloseAsyncOperation::End(asyncOperation);
}

void ComProxyStatefulService::Abort()
{
    service_->Abort();
}

ErrorCode ComProxyStatefulService::GetQueryResult(__out ReplicaStatusQueryResultSPtr & result)
{
    HRESULT hr = E_NOINTERFACE;

    if (internalService_.GetRawPointer() != NULL)
    {
        ComPointer<IFabricStatefulServiceReplicaStatusResult> resultCPtr;
        hr = internalService_->GetStatus(resultCPtr.InitializationAddress());
        Trace.WriteNoise(TraceComponent, "GetStatus returned {0}", hr); //Trace added to investigate RDBug 9191222

        if (resultCPtr.GetRawPointer() == NULL)
        {
            hr = E_NOTIMPL;
        }

        if (SUCCEEDED(hr))
        {
            auto * publicResult = resultCPtr->get_Result();

            result = ReplicaStatusQueryResult::CreateSPtr(publicResult->Kind);
            hr = result->FromPublicApi(*publicResult).ToHResult();
        }
    }

    return ErrorCode::FromHResult(hr);
}
