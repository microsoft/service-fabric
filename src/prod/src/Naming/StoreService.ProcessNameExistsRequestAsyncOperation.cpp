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

    StringLiteral const TraceComponent("ProcessNameExistsRequest");

    StoreService::ProcessNameExistsRequestAsyncOperation::ProcessNameExistsRequestAsyncOperation(
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
        
    ErrorCode StoreService::ProcessNameExistsRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (this->TryGetNameFromRequest(request))
        {       
            this->ProcessRecoveryHeader(move(request));
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }
        
    void StoreService::ProcessNameExistsRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        NameData nameData;
        ErrorCode error = GetNonHierarchyName(nameData);

        if (error.IsSuccess() || error.IsError(ErrorCodeValue::NotFound))
        {
            bool nameExists = error.IsSuccess();

            error = ErrorCodeValue::Success;
            Reply = NamingMessage::GetPeerNameExistsReply(NameExistsReplyMessageBody(nameExists, nameData.ServiceState));
        }

        this->TryComplete(thisSPtr, error);
    }
    
    ErrorCode StoreService::ProcessNameExistsRequestAsyncOperation::GetNonHierarchyName(__out NameData & nameDataResult)
    {
        TransactionSPtr txSPtr;
        EnumerationSPtr enumSPtr;

        auto error = Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (error.IsSuccess())
        {
            _int64 nameSequenceNumber = -1;
            error = Store.TryReadData(
                txSPtr, 
                Constants::NonHierarchyNameEntryType,
                this->NameString,
                nameDataResult,
                nameSequenceNumber); 
        }

        NamingStore::CommitReadOnly(std::move(txSPtr));
        
        WriteNoise(
            TraceComponent,
            "{0} read non-hierarchy name for {1}: error = {2}",
            this->TraceId,
            this->NameString,
            error);

        return error;
    }

}
