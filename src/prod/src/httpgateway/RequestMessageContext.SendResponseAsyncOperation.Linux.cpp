// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace std;
using namespace HttpGateway; 

using namespace Common;


void RequestMessageContext::SendResponseAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    status_code httpStatus;
    string httpStatusLine;

    if (statusCode_ == 0)
    {
	    // need to figureout 
	    // case value evaluates to 2147949518, which cannot be narrowed
//        messageContext_.ErrorCodeToHttpStatus(operationStatus_, httpStatus, httpStatusLine);
	messageContext_->responseUPtr_->set_status_code(status_codes::OK);

    }
    else
    {
	    // description to status 
//         StringUtility::UnicodeToAnsi(statusDescription_, httpStatusLine);
 	messageContext_->responseUPtr_->set_status_code(status_codes::OK);
        
    }

    if (bodyUPtr_)
    {
       messageContext_->responseUPtr_->set_body(*bodyUPtr_);

        if (!NT_SUCCESS(status))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
            return;
        }
    }
	// this will go into async operations.
	http_headers& responseHeader=  messageContext_->responseUPtr_->headers();
	
	// get status code from function  
	responseHeader = *(messageContext_->headerUPtr_);
	requestUPtr_->reply(*(messageContext_->responseUPtr_));

}

void RequestMessageContext::SendResponseAsyncOperation::OnCancel()
{
    TryComplete(thisSPtr, ErrorCode::Success());
}


ErrorCode RequestMessageContext::SendResponseAsyncOperation::End(__in Common::AsyncOperationSPtr const & asyncOperation)
{
    auto operation = AsyncOperation::End<SendResponseAsyncOperation>(asyncOperation);

    return operation->Error;
}

