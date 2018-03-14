// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationHelper
        {
        public:
            static bool IsCannotConnectErrorDuringUserRequest(Common::ErrorCode const & errorCode);
            static bool IsCannotConnectErrorDuringConnect(Common::ErrorCode const & errorCode);
            static bool IsItNonRetryableErrorDuringConnect(Common::ErrorCode const & errorCode);
            static bool IsItNonRetryableErrorDuringUserRequest(Common::ErrorCode const & errorCode);
            static Transport::MessageUPtr CreateReplyMessageWithError(Common::ErrorCode const & errorCode);
        };
    }
}
