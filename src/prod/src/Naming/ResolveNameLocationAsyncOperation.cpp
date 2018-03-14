// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Federation;
    using namespace Store;
    using namespace SystemServices;
    using namespace Reliability;
    using namespace Transport;

    StringLiteral const TraceComponent("ResolveNameLocation");

    ResolveNameLocationAsyncOperation::ResolveNameLocationAsyncOperation(
        __in ServiceResolver & serviceResolver,
        NamingUri const & toResolve,
        Reliability::CacheMode::Enum refreshMode,
        ServiceLocationVersion const & previousVersionUsed,
        NamingServiceCuidCollection const & namingServiceCuids,
        wstring const & baseTraceId,
        FabricActivityHeader const & activityHeader,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & operationRoot)
        : AsyncOperation(callback, operationRoot)
        , ActivityTracedComponent(baseTraceId, activityHeader)
        , serviceResolver_(serviceResolver)
        , toResolve_(toResolve)
        , refreshMode_(refreshMode)
        , previousVersionUsed_(previousVersionUsed)
        , namingServiceCuids_(namingServiceCuids)
        , timeout_(timeout)
    {        
    }

    void ResolveNameLocationAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        vector<VersionedCuid> partitions;
        partitions.push_back(VersionedCuid(
            namingServiceCuids_.HashToNameOwner(toResolve_),
            previousVersionUsed_.FMVersion,
            previousVersionUsed_.Generation));
        WriteNoise(TraceComponent, "{0}: Name {1} is owned by partition {2}", this->TraceId, toResolve_, partitions[0].Cuid);

        auto inner = serviceResolver_.BeginResolveServicePartition(
            partitions,
            refreshMode_,
            ActivityHeader,
            timeout_,
            [this](AsyncOperationSPtr const & asyncOperation) 
            {  
                OnResolveServiceComplete(asyncOperation, false /*expectedCompletedSynchronously*/);
            },
            thisSPtr);
        OnResolveServiceComplete(inner, true /*expectedCompletedSynchronously*/);
    }

    ErrorCode ResolveNameLocationAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out SystemServiceLocation & locationResult,
        __out ServiceLocationVersion & partitionLocationVersionUsed)
    {
        auto casted = AsyncOperation::End<ResolveNameLocationAsyncOperation>(asyncOperation);
        locationResult = casted->serviceLocationResult_;
        partitionLocationVersionUsed = casted->previousVersionUsed_;

        return casted->Error;
    }

    void ResolveNameLocationAsyncOperation::OnResolveServiceComplete(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != asyncOperation->CompletedSynchronously)
        {
            return;
        }

        vector<ServiceTableEntry> serviceEntries;
        GenerationNumber generation;
        ErrorCode error = serviceResolver_.EndResolveServicePartition(asyncOperation, serviceEntries, generation);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} could not resolve Naming Store Service location for {1}: error = {2}",
                this->TraceId,
                toResolve_,
                error);

            // Most errors from the FM on resolve are not retryable with the following exceptions
            switch (error.ReadValue())
            {
                case ErrorCodeValue::ServiceOffline:
                case ErrorCodeValue::PartitionNotFound:
                    // Naming Gateway will retry on this error
                    error = ErrorCodeValue::SystemServiceNotFound;
                    break;
            }
        }
        else
        {
            bool shouldRetry = false;
            wstring endpointLocation;
            auto first = serviceEntries.begin();
            if (first->ServiceReplicaSet.IsPrimaryLocationValid)
            {
                endpointLocation = first->ServiceReplicaSet.PrimaryLocation;
                previousVersionUsed_ = ServiceLocationVersion(first->Version, generation, 0);
            }
            else
            {
                shouldRetry = true;
            }

            if (shouldRetry || !SystemServiceLocation::TryParse(endpointLocation, serviceLocationResult_))
            {
                // Naming Gateway will retry on this error
                error = ErrorCode(ErrorCodeValue::SystemServiceNotFound);
            }
        }

        TryComplete(asyncOperation->Parent, error);
    }
}
