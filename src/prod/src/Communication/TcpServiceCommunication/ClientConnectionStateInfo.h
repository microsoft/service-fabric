// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    static const Common::StringLiteral ClientConnectionStateInfoTraceType("ClientConnectionStateInfo");

    namespace TcpServiceCommunication
    {

        class ClientConnectionStateInfo
            :public Common::TextTraceComponent<Common::TraceTaskCodes::FabricTransport>
        {
            DENY_COPY(ClientConnectionStateInfo)

        public:
            ClientConnectionStateInfo(Transport::ISendTarget::SPtr clientTarget,
                                      ServiceMethodCallDispatcherSPtr const & dispatcher,
                                      std::wstring clientId,
                                       Transport::ReceiverContextUPtr && context)
                                      :clientTarget_(clientTarget),
                                       dispatcher_(dispatcher),
                                       clientId_(clientId),
                                      messsageRecievedBeforeConnected_(),
                                      isConnected_(false),
                                      recieverContext_(move(context))

            {
            }

            bool  IsConnected()
            {
                Common::AcquireReadLock lock(connectionLock_);
                return isConnected_;
            }

            bool SentConnectReply(Transport::MessageUPtr && reply)
            {
                if(recieverContext_)
                {
                    recieverContext_->Reply(move(reply));
                    return true;
                }
                return false;
            }
            
            std::wstring const & Get_ClientId()
            {
                return clientId_;
            }

            Transport::ISendTarget::SPtr Get_SendTarget()
            {
                return clientTarget_;
            }

            std::wstring const & Get_ServiceConnectedTo()
            {
                return dispatcher_->ServiceInfo;
            }

            bool AddToQueueIfNotConnected(Transport::MessageUPtr & message, Transport::ReceiverContextUPtr & context)
            {
                Common::AcquireWriteLock lock(connectionLock_);
                // Keep the msg in temporary queue till the connection Succeded . Dequeue occure in process function
                if (isConnected_)
                {
                    return false;
                }
                WriteNoise(ClientConnectionStateInfoTraceType, "Adding Message  to Service {0} Temporary Queue for Client {1}", dispatcher_->ServiceInfo, clientTarget_->Id());
                messsageRecievedBeforeConnected_.push(std::make_pair(message->Clone(), move(context)));
                return true;
            }

            void SetConnected()
            {
                Common::AcquireWriteLock lock(connectionLock_);
                while (!messsageRecievedBeforeConnected_.empty()){
                    auto pair = std::move(messsageRecievedBeforeConnected_.front());
                    messsageRecievedBeforeConnected_.pop();
                    dispatcher_->AddToQueue(pair.first, pair.second);
                }
                WriteNoise(ClientConnectionStateInfoTraceType, "Listener :{0} Connected to Client :{1}", dispatcher_->ServiceInfo, clientTarget_->Id());
                isConnected_ = true;
            }

        private:

            typedef std::queue<std::pair<Transport::MessageUPtr, Transport::ReceiverContextUPtr>> MessageQueue;
            bool isConnected_;
            MessageQueue messsageRecievedBeforeConnected_;
            Common::RwLock connectionLock_;
            ServiceMethodCallDispatcherSPtr dispatcher_;
            Transport::ISendTarget::SPtr clientTarget_;
            Transport::ReceiverContextUPtr recieverContext_;
            std::wstring clientId_;
        };
        typedef std::shared_ptr<ClientConnectionStateInfo> ClientConnectionStateInfoSPtr;
    }
}
