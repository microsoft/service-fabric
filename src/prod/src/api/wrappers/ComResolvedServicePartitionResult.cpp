// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

    
using namespace std;
using namespace Common;
using namespace Api;
using namespace Reliability;

ComResolvedServicePartitionResult::ComResolvedServicePartitionResult(
    IResolvedServicePartitionResultPtr const & impl)
    : impl_(impl)
    , heap_()
    , partition_()
    , mainEndpoint_()
    , lock_()
{
}

ComResolvedServicePartitionResult::ComResolvedServicePartitionResult()
    : impl_()
    , heap_()
    , partition_()
    , mainEndpoint_()
    , lock_()
{
}

const FABRIC_RESOLVED_SERVICE_PARTITION * STDMETHODCALLTYPE ComResolvedServicePartitionResult::get_Partition()
{
    {
        AcquireReadLock lock(lock_);

        if (partition_)
        {
            return partition_.GetRawPointer();
        }
    }

    {
        AcquireWriteLock lock(lock_);

        if (!partition_)
        {
            partition_ = heap_.AddItem<FABRIC_RESOLVED_SERVICE_PARTITION>();
            impl_->GetPartition(heap_, *partition_);
        }

        return partition_.GetRawPointer();
    }
}

HRESULT STDMETHODCALLTYPE ComResolvedServicePartitionResult::GetEndpoint(
    __out /* [out, retval] */ const FABRIC_RESOLVED_SERVICE_ENDPOINT ** endpoint)
{
    if (endpoint == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    {
        AcquireReadLock lock(lock_);

        if (mainEndpoint_)
        {
            *endpoint = mainEndpoint_.GetRawPointer();

            return ComUtility::OnPublicApiReturn(S_OK);
        }
    }
    
    {
        AcquireWriteLock lock(lock_);

        if (!mainEndpoint_)
        {
            // Filter the native partition endpoints and choose
            // the primary OR a random stateless instace.
            // For stateless, the method will return the same stateless instance;
            // however, if multiple ComResolvedServicePartition are built on top of the
            // same ResolvedServicePartition, they may return different results.
            Reliability::ServiceReplicaSet const& serviceReplicaSet = impl_->GetServiceReplicaSet();
            if (serviceReplicaSet.IsStateful)
            {
                if (serviceReplicaSet.IsPrimaryLocationValid)
                {
                    mainEndpoint_ = heap_.AddItem<FABRIC_RESOLVED_SERVICE_ENDPOINT>();
                    mainEndpoint_->Address = heap_.AddString(serviceReplicaSet.PrimaryLocation);
                    mainEndpoint_->Role = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATEFUL_PRIMARY;
                }
                else
                {
                    return ComUtility::OnPublicApiReturn(FABRIC_E_REPLICA_DOES_NOT_EXIST);
                }
            }
            else
            {
                size_t instances = serviceReplicaSet.ReplicaLocations.size();
                if (instances > 0)
                {
                    Random r;
                    size_t statelessEndpointIndex = static_cast<size_t>(r.Next(static_cast<int>(instances)));

                    mainEndpoint_ = heap_.AddItem<FABRIC_RESOLVED_SERVICE_ENDPOINT>();
                    mainEndpoint_->Address = heap_.AddString(serviceReplicaSet.ReplicaLocations[statelessEndpointIndex]);
                    mainEndpoint_->Role = FABRIC_SERVICE_ENDPOINT_ROLE::FABRIC_SERVICE_ROLE_STATELESS;
                }
                else
                {
                    return ComUtility::OnPublicApiReturn(FABRIC_E_SERVICE_OFFLINE);
                }
            }
        }

        *endpoint = mainEndpoint_.GetRawPointer();

        return ComUtility::OnPublicApiReturn(S_OK);
    }
}

HRESULT ComResolvedServicePartitionResult::CompareVersion(
    __in /* [in] */ IFabricResolvedServicePartitionResult * other,
    __out /* [out, retval] */ LONG * compareResult)
{
    if (other == NULL || compareResult == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    
    ComPointer<ComResolvedServicePartitionResult> otherRsp;
    otherRsp.SetAndAddRef(dynamic_cast<ComResolvedServicePartitionResult *>(other));
    if (!otherRsp)
    {
        return ComUtility::OnPublicApiReturn(E_INVALIDARG);
    }

    auto error = impl_->CompareVersion(otherRsp->get_Impl(), compareResult);
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

HRESULT STDMETHODCALLTYPE ComResolvedServicePartitionResult::get_FMVersion( 
    /* [out] */ LONGLONG * value)
{
    if (value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }
    
    auto error = impl_->GetFMVersion(value);
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

HRESULT STDMETHODCALLTYPE ComResolvedServicePartitionResult::get_Generation( 
    /* [out] */ LONGLONG * value)
{
    if (value == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    auto error = impl_->GetGeneration(value);
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

