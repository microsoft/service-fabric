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
    auto hr = service_->QueryInterface(IID_IFabricInternalStatefulServiceReplica2, internalService_.VoidInitializationAddress());

    if (SUCCEEDED(hr))
    {
        Trace.WriteNoise(TraceComponent, "IFabricInternalStatefulServiceReplica2 supported");
    }
    else
    {
        if (hr == E_NOINTERFACE)
        {
            Trace.WriteNoise(TraceComponent, "IFabricInternalStatefulServiceReplica2 not supported");
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "Failed to QI for IFabricInternalStatefulServiceReplica2: hr={0}", hr);
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

        if (SUCCEEDED(hr))
        {
            if (resultCPtr.GetRawPointer() != NULL)
            {
                auto * publicResult = resultCPtr->get_Result();

                result = ReplicaStatusQueryResult::CreateSPtr(publicResult->Kind);
                hr = result->FromPublicApi(*publicResult).ToHResult();
            }
            else
            {
                Trace.WriteWarning(TraceComponent, "IFabricStatefulServiceReplicaStatusResult is null");

                hr = E_NOTIMPL;
            }
        }
        else
        {
            Trace.WriteWarning(TraceComponent, "IFabricInternalStatefulServiceReplica2::GetStatus failed: {0}", hr);
        }
    }

    return ErrorCode::FromHResult(hr);
}

void ComProxyStatefulService::UpdateInitializationData(vector<byte> const & initData)
{
    if (internalService_.GetRawPointer() != NULL)
    {
        auto hr = internalService_->UpdateInitializationData(static_cast<ULONG>(initData.size()), initData.data());

        if (FAILED(hr))
        {
            Trace.WriteWarning(TraceComponent, "IFabricInternalStatefulServiceReplica2::UpdateInitializationData failed: {0}", hr);
        }
    }
    else
    {
        Trace.WriteInfo(TraceComponent, "IFabricInternalStatefulServiceReplica2 interface not found");
    }
}
