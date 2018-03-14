// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class RaToRapTransportStub : public IRaToRapTransport
            {
            public:
                typedef AsyncApiStub<std::tuple<Transport::MessageUPtr, std::wstring>, Transport::MessageUPtr> RequestReplyApiStub;
                RequestReplyApiStub RequestReplyApi;

                RaToRapTransportStub(bool enableTrackOfMessages) :
                    RequestReplyApi(L"RequestReply"),
                    enableTrackOfMessages_(enableTrackOfMessages)
                {
                }

                void SendOneWay(
                    std::wstring const & target,
                    Transport::MessageUPtr && message,
                    Common::TimeSpan) override
                {
                    if (!enableTrackOfMessages_)
                    {
                        return;
                    }

                    Messages[target].push_back(std::move(message));
                }

                void RegisterMessageHandler(Transport::Actor::Enum, Transport::IpcMessageHandler const &, bool) override
                {

                }

                Common::AsyncOperationSPtr BeginRequest(
                    Transport::MessageUPtr && request,
                    std::wstring const & target,
                    Common::TimeSpan, /*timeout*/
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    auto targetCopy = target;
                    std::tuple<Transport::MessageUPtr, std::wstring> tuple2(std::move(request), std::move(targetCopy));
                    return RequestReplyApi.OnCall2(std::move(tuple2), callback, parent);
                }

                Common::ErrorCode EndRequest(
                    Common::AsyncOperationSPtr const & operation,
                    Transport::MessageUPtr & reply) override
                {
                    return RequestReplyApiStub::End(operation, reply);
                }

                std::map<std::wstring, std::vector<Transport::MessageUPtr>> Messages;
            private:
                bool enableTrackOfMessages_;
            };
        }

    }
}
