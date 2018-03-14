// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace Store;
    using namespace std;

    StringLiteral const TraceComponent("ProcessServiceExists");

    StoreService::ProcessServiceExistsRequestAsyncOperation::ProcessServiceExistsRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
    {
    }

    ErrorCode StoreService::ProcessServiceExistsRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        ServiceCheckRequestBody body;
        if (request->GetBody(body))
        {
            if (body.IsPrefixMatch)
            {
                TRACE_LEVEL_AND_TESTASSERT(
                    WriteError,
                    TraceComponent,
                    "{0}: prefix match not supported at Name Owner: {1}",
                    this->TraceId,
                    body.Name);

                return ErrorCodeValue::InvalidMessage;
            }
            else
            {
                this->SetName(body.Name);
                return ErrorCodeValue::Success;
            }
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessServiceExistsRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {        
        this->HandleRequestFromCache(thisSPtr);
    }

    void StoreService::ProcessServiceExistsRequestAsyncOperation::HandleRequestFromCache(AsyncOperationSPtr const & thisSPtr)
    {
        auto const & name = this->NameString;

        StoreServicePsdCacheEntrySPtr cacheEntry;
        if (this->PsdCache.TryGetCacheEntry(name, cacheEntry))
        {
            WriteInfo(
                TraceComponent,
                "{0}: Cached PSD at name {1} is {2}",
                this->TraceId,
                name,
                *(cacheEntry->Psd));

            Reply = NamingMessage::GetServiceCheckReply(ServiceCheckReplyBody(*(cacheEntry->Psd)));

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else if (!this->PsdCache.IsCacheLimitEnabled)
        {
            WriteInfo(
                TraceComponent,
                "{0}: Cached PSD miss {1}",
                this->TraceId,
                name);

            this->SetServiceDoesNotExistReply();

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            this->HandleRequestFromDisk(thisSPtr);
        }
    }

    void StoreService::ProcessServiceExistsRequestAsyncOperation::HandleRequestFromDisk(AsyncOperationSPtr const & thisSPtr)
    {
        // This path replies to the Naming Gateway, which needs service metadata found in the PSD.
        //
        
        UserServiceState::Enum state;
        PartitionedServiceDescriptor psd;
        ErrorCode error = this->GetUserServiceMetadata(state, psd);

        if (error.IsSuccess())
        {
            switch (state)
            {
            case UserServiceState::Deleting:
            case UserServiceState::ForceDeleting:
            case UserServiceState::Created:
            case UserServiceState::Updating:
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: Service PSD at name {1} is {2} ({3})",
                    this->TraceId,
                    this->NameString,
                    psd,
                    state);

                auto body = ServiceCheckReplyBody(move(psd));
                Reply = NamingMessage::GetServiceCheckReply(body);

                if (this->PsdCache.IsCacheLimitEnabled)
                {
                    auto cacheEntry = make_shared<StoreServicePsdCacheEntry>(body.TakePsd());
                    this->PsdCache.UpdateCacheEntry(cacheEntry);
                }

                break;
            }

            default:
                WriteInfo(
                    TraceComponent,
                    "{0}: Service PSD at name {1} is not created ({2})",
                    this->TraceId,
                    this->NameString,
                    state);

                error = ErrorCodeValue::UserServiceNotFound;
            }
        }

        if (error.IsError(ErrorCodeValue::NameNotFound) || error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            this->SetServiceDoesNotExistReply();

            error = ErrorCodeValue::Success;
        }

        this->TryComplete(thisSPtr, error);
    }

    ErrorCode StoreService::ProcessServiceExistsRequestAsyncOperation::GetUserServiceMetadata(
        __out UserServiceState::Enum & serviceState,
        __out PartitionedServiceDescriptor & psdResult)
    {
        TransactionSPtr txSPtr;
        ErrorCode error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (error.IsSuccess())
        {
            error = this->InternalGetUserServiceStateAtNameOwner(txSPtr, serviceState);
        }

        if (!error.IsSuccess() || serviceState != UserServiceState::Created)
        {
            NamingStore::CommitReadOnly(move(txSPtr));
            return error;
        }

        PartitionedServiceDescriptor psd;
        _int64 psdSequenceNumber;
        error = this->Store.TryReadData(txSPtr, Constants::UserServiceDataType, this->NameString, psd, psdSequenceNumber);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to read PSD for {1}: error={2}",
                this->TraceId,
                this->NameString,
                error);
            return (error.IsError(ErrorCodeValue::NotFound) ? ErrorCodeValue::UserServiceNotFound : error);
        }

        psd.Version = psdSequenceNumber;
        psdResult = move(psd);

        return error;
    }
    
    ErrorCode StoreService::ProcessServiceExistsRequestAsyncOperation::InternalGetUserServiceStateAtNameOwner(
        TransactionSPtr const & txSPtr,
        __out UserServiceState::Enum & serviceState)
    {
        NameData nameOwnerEntry;
        _int64 nameSequenceNumber = -1;
        ErrorCode error = this->Store.TryReadData(
            txSPtr,
            Constants::NonHierarchyNameEntryType,
            this->NameString,
            nameOwnerEntry,
            nameSequenceNumber);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to read Name {1}: error = {2}",
                this->TraceId,
                this->NameString,
                error);

            return (error.IsError(ErrorCodeValue::NotFound) ? ErrorCodeValue::NameNotFound : error);
        }

        serviceState = nameOwnerEntry.ServiceState;

        return error;
    }

    void StoreService::ProcessServiceExistsRequestAsyncOperation::SetServiceDoesNotExistReply()
    {
        // The LSN is used by the polling-based service notification mechanism 
        // to order non-existent service faults
        //
        FABRIC_SEQUENCE_NUMBER lsn = this->PsdCache.GetServicesLsn();

        Reply = NamingMessage::GetServiceCheckReply(ServiceCheckReplyBody(lsn));
    }
}
