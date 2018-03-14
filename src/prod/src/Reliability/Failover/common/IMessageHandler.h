// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class IMessageHandler
    {
    public:
        // Process a request response received over federation
        // The metadata is what was specified when the job item context was created
        virtual void ProcessRequestResponseRequest(
            IMessageMetadata const * messageMetadata,
            Transport::MessageUPtr && requestPtr,
            Federation::RequestReceiverContextUPtr && context) = 0;

        // Process a one way message over IPC
        // The metadata is what was specified when the job item context was created
        virtual void ProcessIpcMessage(
            IMessageMetadata const * messageMetadata, 
            Transport::MessageUPtr && message, 
            Transport::IpcReceiverContextUPtr && receiverContext) = 0;

        // Process a oneway message over federation
        // The metadata is what was specified when the job item context was created
        virtual void ProcessRequest(
            IMessageMetadata const * messageMetadata,
            Transport::MessageUPtr && requestPtr, 
            Federation::OneWayReceiverContextUPtr && context) = 0;

        virtual ~IMessageHandler() {}
    };
}
