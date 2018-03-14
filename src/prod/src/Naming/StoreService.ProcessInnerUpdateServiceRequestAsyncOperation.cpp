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

    StringLiteral const TraceComponent("ProcessInnerUpdateService");

    StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::ProcessInnerUpdateServiceRequestAsyncOperation(
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
        , updateDescription_()
        , psd_()
        , addedCuids_()
        , removedCuids_()
    {
    }

    ErrorCode StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        UpdateServiceRequestBody body;
        if (request->GetBody(body))
        {
            this->SetName(body.ServiceName);
            updateDescription_ = body.UpdateDescription;

            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        this->Properties.Trace.NOInnerUpdateServiceRequestReceiveComplete(
            this->TraceId,
            this->Node.Id.ToString(), 
            this->Node.InstanceId,
            this->NameString);

        if (this->TryAcceptRequestAtNameOwner(thisSPtr))
        {
            // Assume success - overwrite body on validation failure
            Reply = NamingMessage::GetInnerUpdateServiceOperationReply(UpdateServiceReplyBody());

            this->StartUpdateService(thisSPtr);
        }
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::StartUpdateService(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error = this->GetUserServiceMetadata(psd_);

        if (error.IsSuccess())
        {
            bool updated = false;
            error = this->TryUpdateServiceDescription(updated);

            if (error.IsSuccess())
            {
                if (updated)
                {
                    this->StartRequestToFM(thisSPtr);
                }
                else
                {
                    // Nothing to do - just complete successfully and don't send to FM.
                    //
                    this->TryComplete(thisSPtr, error);
                }
            }
            else
            {
                // Return success to AO (we don't want it to retry) but indicate 
                // a validation error in the reply body
                //
                Reply = NamingMessage::GetInnerUpdateServiceOperationReply(UpdateServiceReplyBody(error.ReadValue(), error.TakeMessage()));
                this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::StartRequestToFM(AsyncOperationSPtr const & thisSPtr)
    {
        // AdminClient traces ServiceDescription at Info level already, so only trace PSD specific details at Noise
        //
        WriteNoise(
            TraceComponent, 
            "{0}: UpdateService request to FM: desc={1} added={2} removed={3}", 
            this->TraceId, 
            psd_.Service, 
            addedCuids_, 
            removedCuids_);

        auto inner = this->Properties.AdminClient.BeginUpdateService(
            psd_.Service,
            move(addedCuids_),
            move(removedCuids_),
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnRequestToFMComplete(operation, false); },
            thisSPtr); 

        this->Properties.Trace.NOInnerUpdateServiceToFMSendComplete(
            this->TraceId, 
            this->Node.Id.ToString(), 
            this->Node.InstanceId, 
            this->NameString);  

        this->OnRequestToFMComplete(inner, true);
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::OnRequestToFMComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        UpdateServiceReplyMessageBody reply;
        ErrorCode error = this->Properties.AdminClient.EndUpdateService(operation, reply);

        if (error.IsSuccess())
        {
            error = reply.Error;
        }

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else
        {
            this->FinishUpdateService(thisSPtr);
        }
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::FinishUpdateService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<WriteServiceAtNameOwnerAsyncOperation>(
            this->Name,
            this->Store,
            true, /*isCreationComplete*/
            true, /*shouldUpdatePsd*/ 
            psd_,
            this->ActivityHeader,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnWriteServiceComplete(operation, false); },
            thisSPtr);
        this->OnWriteServiceComplete(operation, true);
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::OnWriteServiceComplete(
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

        if (error.IsSuccess())
        {
            psd_.Version = lsn;

            auto cacheEntry = make_shared<StoreServicePsdCacheEntry>(move(psd_));
            this->PsdCache.UpdateCacheEntry(cacheEntry, lsn);
        }

        this->TryComplete(thisSPtr, error);
    }

    ErrorCode StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::GetUserServiceMetadata(__out PartitionedServiceDescriptor & psdResult)
    {
        TransactionSPtr txSPtr;
        auto error = this->Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        NameData nameOwnerEntry;
        _int64 nameSequenceNumber = -1;
        error = this->Store.TryReadData(
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
        else if (nameOwnerEntry.ServiceState != UserServiceState::Created)
        {
            return ErrorCodeValue::UserServiceNotFound;
        }

        PartitionedServiceDescriptor psd;
        _int64 psdSequenceNumber = -1;
        error = this->Store.TryReadData(txSPtr, Constants::UserServiceDataType, this->NameString, psd, psdSequenceNumber);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Service at {1} does not exist.",
                this->TraceId,
                this->NameString);
            return (error.IsError(ErrorCodeValue::NotFound) ? ErrorCodeValue::UserServiceNotFound : error);
        }

        psdResult = move(psd);

        return error;
    }

    ErrorCode StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::TryUpdateServiceDescription(__out bool & updated)
    {
        ErrorCode error(ErrorCodeValue::Success);

        // Modify the loaded PSD's ServiceDescription directly since the AO will drive retries,
        // which means that the PSD will be re-loaded on retry.
        //
        error = updateDescription_.TryUpdateServiceDescription(psd_, updated, addedCuids_, removedCuids_); // out

        if (!error.IsSuccess())
        {
            return error;
        }

        if (!updated)
        {
            WriteInfo(
                TraceComponent,
                "{0}: no service updates for {1}: update=[{2}]",
                this->TraceId,
                this->NameString,
                updateDescription_);

            return error;
        }

        error = psd_.Validate();

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: update service validation failed for {1}: error={2}",
                this->TraceId,
                this->NameString,
                error);

            return error;
        }

        if (!addedCuids_.empty() && !removedCuids_.empty())
        {
            // Naming supports both adding and removing partitions in the same
            // update, but FM currently does not. There is no technical limitation
            // against accepting both in the same operation, but not doing so
            // makes the FM implementation simpler.
            //
            WriteWarning(
                TraceComponent,
                "{0}: simultaneous add+remove of partitions not supported for {1}: update=[{2}]",
                this->TraceId,
                this->NameString,
                updateDescription_);

            return ErrorCode(
                ErrorCodeValue::InvalidArgument, 
                wformatString(GET_NS_RC( Update_Unsupported_AddRemove ), this->NameString));
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: update service validated for {1}: update=[{2}]",
                this->TraceId,
                this->NameString,
                updateDescription_);
        }

        return error;
    }

    void StoreService::ProcessInnerUpdateServiceRequestAsyncOperation::OnCompleted()
    {
        StoreService::ProcessRequestAsyncOperation::OnCompleted();

        this->Properties.Trace.NOInnerUpdateServiceRequestProcessComplete(
                this->TraceId, 
                this->Node.Id.ToString(),
                this->Node.InstanceId,
                this->NameString);
    }
}
