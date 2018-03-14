// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Communication::TcpServiceCommunication;

bool ServiceCommunicationHelper::IsCannotConnectErrorDuringUserRequest(ErrorCode const & error)
{
    return
        !IsItNonRetryableErrorDuringUserRequest(error);
}

bool ServiceCommunicationHelper::IsCannotConnectErrorDuringConnect(ErrorCode const & error)
{
    return
        !IsItNonRetryableErrorDuringConnect(error);
}

MessageUPtr ServiceCommunicationHelper::CreateReplyMessageWithError(ErrorCode const & errorCode)
{
    MessageUPtr reply = make_unique<Message>();
    reply->Headers.Add(ServiceCommunicationErrorHeader(errorCode.ReadValue()));
    return reply;
 }

bool ServiceCommunicationHelper::IsItNonRetryableErrorDuringConnect(ErrorCode const & error)
{
    //If its Timeout or MessageTooLarge for Connect Request , then we consider it as Retryable Error after re-resolving. 
    //For case like if replicator is the listening server, then we see Timeout as error.
    //Another case is where unknown transport is listening and sent large message , then we see error as MessageTooLarge
    return
        (error.IsError(ErrorCodeValue::ConnectionDenied) ||
            error.IsError(ErrorCodeValue::ServerAuthenticationFailed) ||
            error.IsError(ErrorCodeValue::InvalidCredentials) ||
            error.IsError(ErrorCodeValue::EndpointNotFound) ||
            error.IsError(ErrorCodeValue::NotPrimary) ||
            error.IsError(ErrorCodeValue::ServiceTooBusy));
}

bool ServiceCommunicationHelper::IsItNonRetryableErrorDuringUserRequest(ErrorCode const & error)
{
    return IsItNonRetryableErrorDuringConnect(error) ||
        error.IsError(ErrorCodeValue::Timeout) ||
            error.IsError(ErrorCodeValue::MessageTooLarge);
}
