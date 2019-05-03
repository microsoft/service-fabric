// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace Store;

    StringLiteral const TraceComponent("ProcessInnerCreateService");

    StoreService::ProcessInnerCreateServiceRequestAsyncOperation::ProcessInnerCreateServiceRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        __in StoreServiceProperties & properties,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
        , psd_()
        , isRebuildFromFM_(false)
        , revertError_()
    {
    }

    ErrorCode StoreService::ProcessInnerCreateServiceRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        CreateServiceMessageBody body;
        if (request->GetBody(body))
        {
            this->SetName(body.NamingUri);
            this->MutableServiceDescriptor = move(const_cast<PartitionedServiceDescriptor &>(body.PartitionedServiceDescriptor));

            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.NOInnerCreateServiceRequestReceiveComplete(
            this->TraceId,
            this->Node.Id.ToString(), 
            this->Node.InstanceId,
            this->NameString);

        if (this->TryAcceptRequestAtNameOwner(thisSPtr))
        {
            // Just send an empty reply
            Reply = NamingMessage::GetInnerCreateServiceOperationReply();

            this->StartCreateService(thisSPtr);
        }
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::StartCreateService(AsyncOperationSPtr const & thisSPtr)
    {
        this->MutableServiceDescriptor.MutableService.Instance = DateTime::Now().Ticks;

        auto operation = AsyncOperation::CreateAndStart<WriteServiceAtNameOwnerAsyncOperation>(
            this->Name,
            this->Store,
            false /*isCreationComplete*/,
            true /*shouldUpdatePsd*/,
            this->ServiceDescriptor,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnTentativeWriteComplete(operation, false); },
            thisSPtr);
        this->OnTentativeWriteComplete(operation, true);
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::OnTentativeWriteComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        FABRIC_SEQUENCE_NUMBER lsn = -1;
        ErrorCode error = WriteServiceAtNameOwnerAsyncOperation::End(operation, lsn);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            // Set version for read from cache
            this->MutableServiceDescriptor.Version = lsn;

            // Once the request is sent to FM, it may start broadcasting endpoint updates before
            // Naming Service receives any reply, so make the PSD available in the cache for service notification
            // processing. The cached PSD will be updated with the latest LSN upon receiving the FM reply.
            // Clients will invalidate their PSD cache if an FM rebuild occurred that changed the PSD.
            //
            // Note that this still does not guarantee a valid PSD for service notification processine, since
            // the FM could have rebuilt the service. In which case, this pre-emptively cached PSD will not
            // match the PSD rebuilt from FM.
            //
            auto cacheEntry = make_shared<StoreServicePsdCacheEntry>(PartitionedServiceDescriptor(this->MutableServiceDescriptor));
            this->PsdCache.UpdateCacheEntry(cacheEntry, lsn);

            this->StartRequestToFM(thisSPtr);
        }
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::StartRequestToFM(AsyncOperationSPtr const & thisSPtr)
    {
        PartitionedServiceDescriptor const & psd  = this->ServiceDescriptor;
        
        // AdminClient traces ServiceDescription at Info level already, so only trace PSD specific details at Noise
        //
        WriteNoise(TraceComponent, "{0}: CreateService request to FM: {1}", this->TraceId, psd);

        auto inner = this->Properties.AdminClient.BeginCreateService(
            psd.Service,
            psd.CreateConsistencyUnitDescriptions(),
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRequestToFMComplete(operation, false); },
            thisSPtr); 

        this->Properties.Trace.NOInnerCreateServiceToFMSendComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);  

        this->OnRequestToFMComplete(inner, true);
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::OnRequestToFMComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        CreateServiceReplyMessageBody replyBody;
        ErrorCode error = this->Properties.AdminClient.EndCreateService(operation, replyBody);

        if (error.IsSuccess())
        {
            error = replyBody.ErrorCodeValue;
        }

        if (error.IsError(ErrorCodeValue::FMServiceAlreadyExists))
        {
            this->GetServiceDescriptionFromFM(thisSPtr);
        }
        else
        {
            if (this->Store.NeedsReversion(error))
            {
                revertError_ = error;
                this->RevertTentativeCreate(thisSPtr);
            }
            else if (!error.IsSuccess())
            {
                this->TryComplete(thisSPtr, error);
            }
            else
            {
                this->FinishCreateService(thisSPtr);
            }
        }
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::GetServiceDescriptionFromFM(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = this->Properties.AdminClient.BeginGetServiceDescriptionWithCuids(
            this->NameString,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation) { this->OnGetServiceDescriptionComplete(operation, false); },
            thisSPtr);
        this->OnGetServiceDescriptionComplete(operation, true);
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::OnGetServiceDescriptionComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        ServiceDescriptionReplyMessageBody replyBody;
        auto error = this->Properties.AdminClient.EndGetServiceDescription(operation, replyBody);

        if (error.IsSuccess())
        {
            error = replyBody.Error;
        }

        if (error.IsSuccess())
        {
            // Tactical consistency approach between Naming and FM in the event of data loss in Naming.
            // If the service already exists in the FM, ensure that Naming uses the same CUIDs and
            // ServiceDescription.
            //
            PartitionedServiceDescriptor psd;
            error = PartitionedServiceDescriptor::Create(replyBody.ServiceDescription, replyBody.ConsistencyUnitDescriptions, psd);

            if (error.IsSuccess())
            {
                WriteInfo(TraceComponent, "{0}: Rebuilding service from FM: {1}", this->TraceId, psd);

                isRebuildFromFM_ = true;

                this->MutableServiceDescriptor = move(psd);
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0}: Cannot create PSD from: {1} {2}", 
                    this->TraceId, 
                    replyBody.ServiceDescription, 
                    replyBody.ConsistencyUnitDescriptions);
            }
        }

        if (this->Store.NeedsReversion(error))
        {
            revertError_ = error;
            this->RevertTentativeCreate(thisSPtr);
        }
        else if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            this->FinishCreateService(thisSPtr);
        }
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::FinishCreateService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<WriteServiceAtNameOwnerAsyncOperation>(
            this->Name,
            this->Store,
            true /*isCreationComplete*/,
            isRebuildFromFM_ /*shouldUpdatePsd*/,
            this->ServiceDescriptor,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnWriteServiceComplete(operation, false); },
            thisSPtr);
        this->OnWriteServiceComplete(operation, true);
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::OnWriteServiceComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        FABRIC_SEQUENCE_NUMBER lsn = -1;
        ErrorCode error = WriteServiceAtNameOwnerAsyncOperation::End(operation, lsn);

        if (error.IsSuccess() && isRebuildFromFM_)
        {
            error = ErrorCodeValue::UserServiceAlreadyExists;
        }

        if (error.IsSuccess() || error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
        {
            // If we are not rebuilding from FM, then only the service state flag on
            // the name is updated (i.e. the PSD itself is not updated)
            //
            if (isRebuildFromFM_)
            {
                this->MutableServiceDescriptor.Version = lsn;
            }

            auto cacheEntry = make_shared<StoreServicePsdCacheEntry>(move(this->MutableServiceDescriptor));
            this->PsdCache.UpdateCacheEntry(cacheEntry, lsn);
        }

        this->TryComplete(thisSPtr, error);
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::RevertTentativeCreate(AsyncOperationSPtr const & thisSPtr)
    {
        this->PsdCache.RemoveCacheEntry(this->NameString, 0);

        auto inner = AsyncOperation::CreateAndStart<RemoveServiceAtNameOwnerAsyncOperation>(
            this->Name,
            this->Store,
            true /*isDeletionComplete*/,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRevertTentativeComplete(operation, false); },
            thisSPtr);
        this->OnRevertTentativeComplete(inner, true);
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::OnRevertTentativeComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        ErrorCode error = RemoveServiceAtNameOwnerAsyncOperation::End(operation);

        if (error.IsError(ErrorCodeValue::UserServiceNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            this->CompleteOrScheduleRetry(thisSPtr, move(error),
                [this](AsyncOperationSPtr const & thisSPtr) { this->RevertTentativeCreate(thisSPtr); });
        }
        else
        {
            this->TryComplete(thisSPtr, revertError_);
        }
    }

    void StoreService::ProcessInnerCreateServiceRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        this->Properties.Trace.NOInnerCreateServiceRequestProcessComplete(
                this->TraceId, 
                this->Node.Id.ToString(),
                this->Node.InstanceId,
                this->NameString);
    }
}
