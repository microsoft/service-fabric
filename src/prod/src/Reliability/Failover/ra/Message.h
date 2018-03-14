// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class RAMessage
        {
        public:
            RAMessage(
                std::wstring action)
                : actionHeader_(action) 
            {}

            static RAMessage const & GetReplicaOpen() { return ReplicaOpen; }
            static RAMessage const & GetReplicaClose() { return ReplicaClose; }

            static RAMessage const & GetStatefulServiceReopen() { return StatefulServiceReopen; }

            static RAMessage const & GetUpdateConfiguration() { return UpdateConfiguration; }
            static RAMessage const & GetReplicatorBuildIdleReplica() { return ReplicatorBuildIdleReplica; }
            static RAMessage const & GetReplicatorRemoveIdleReplica() { return ReplicatorRemoveIdleReplica; }
            static RAMessage const & GetReplicatorGetStatus() { return ReplicatorGetStatus; }
            static RAMessage const & GetReplicatorUpdateEpochAndGetStatus() { return ReplicatorUpdateEpochAndGetStatus; }
            static RAMessage const & GetCancelCatchupReplicaSet() { return CancelCatchupReplicaSet; }
            static RAMessage const & GetReportLoad() { return ReportLoad; }
            static RAMessage const & GetReportFault() { return ReportFault; }
            static RAMessage const & GetReplicaOpenReply() { return ReplicaOpenReply; }
            static RAMessage const & GetReplicaCloseReply() { return ReplicaCloseReply; }
            static RAMessage const & GetStatefulServiceReopenReply() { return StatefulServiceReopenReply; }
            static RAMessage const & GetUpdateConfigurationReply() { return UpdateConfigurationReply; }
            static RAMessage const & GetReplicatorBuildIdleReplicaReply() { return ReplicatorBuildIdleReplicaReply; }
            static RAMessage const & GetReplicatorRemoveIdleReplicaReply() { return ReplicatorRemoveIdleReplicaReply; }
            static RAMessage const & GetReplicatorGetStatusReply() { return ReplicatorGetStatusReply; }
            static RAMessage const & GetReplicatorUpdateEpochAndGetStatusReply() { return ReplicatorUpdateEpochAndGetStatusReply; }
            static RAMessage const & GetCancelCatchupReplicaSetReply() { return CancelCatchupReplicaSetReply; }
            static RAMessage const & GetReplicaEndpointUpdated() { return ReplicaEndpointUpdated; }
            static RAMessage const & GetReplicaEndpointUpdatedReply() { return ReplicaEndpointUpdatedReply; }

            static RAMessage const & GetReadWriteStatusRevokedNotification() { return ReadWriteStatusRevokedNotification; }
            static RAMessage const & GetReadWriteStatusRevokedNotificationReply() { return ReadWriteStatusRevokedNotificationReply; }

            static RAMessage const & GetRAPQuery() { return RAPQuery; }
            static RAMessage const & GetRAPQueryReply() { return RAPQueryReply; }

            static RAMessage const & GetUpdateServiceDescription() { return UpdateServiceDescription; }
            static RAMessage const & GetUpdateServiceDescriptionReply() { return UpdateServiceDescriptionReply;  }

            // Creates a message without a body
            Transport::MessageUPtr CreateMessage() const
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                AddHeaders(*message);

                return std::move(message);
            }

            // Creates a message with the specified body
            template <class T> 
            Transport::MessageUPtr CreateMessage(T const& t) const
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(t));

                AddHeaders(*message);

                return std::move(message);
            }

            // Creates a message with the specified body and adds partitionId to Message property if UnreliableTransport is enabled.
            template <class T>
            Transport::MessageUPtr CreateMessage(T const& t, Common::Guid const & partitionId) const
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(t));
                Transport::UnreliableTransport::AddPartitionIdToMessageProperty(*message, partitionId);
                AddHeaders(*message);

                return std::move(message);
            }

            __declspec (property(get=get_Action)) const std::wstring & Action;
            std::wstring const& get_Action() const { return actionHeader_.Action; } ;

            static Transport::Actor::Enum const Actor;

        private:

            void AddHeaders(Transport::Message & message) const;
            
            static Common::Global<RAMessage> ReplicaOpen;
            static Common::Global<RAMessage> ReplicaClose;
            
            static Common::Global<RAMessage> StatefulServiceReopen;
            static Common::Global<RAMessage> UpdateConfiguration;
            static Common::Global<RAMessage> ReplicatorBuildIdleReplica;
            static Common::Global<RAMessage> ReplicatorRemoveIdleReplica;
            static Common::Global<RAMessage> ReplicatorGetStatus;
            static Common::Global<RAMessage> ReplicatorUpdateEpochAndGetStatus;
            static Common::Global<RAMessage> CancelCatchupReplicaSet;

            static Common::Global<RAMessage> ReplicaOpenReply;
            static Common::Global<RAMessage> ReplicaCloseReply;
            static Common::Global<RAMessage> StatefulServiceReopenReply;
            static Common::Global<RAMessage> UpdateConfigurationReply;
            static Common::Global<RAMessage> ReplicatorBuildIdleReplicaReply;
            static Common::Global<RAMessage> ReplicatorRemoveIdleReplicaReply;
            static Common::Global<RAMessage> ReplicatorGetStatusReply;
            static Common::Global<RAMessage> ReplicatorUpdateEpochAndGetStatusReply;
            static Common::Global<RAMessage> CancelCatchupReplicaSetReply;
            static Common::Global<RAMessage> ReportLoad;
            static Common::Global<RAMessage> ReportFault;

            static Common::Global<RAMessage> ReplicaEndpointUpdated;
            static Common::Global<RAMessage> ReplicaEndpointUpdatedReply;

            static Common::Global<RAMessage> ReadWriteStatusRevokedNotification;
            static Common::Global<RAMessage> ReadWriteStatusRevokedNotificationReply;

            static Common::Global<RAMessage> RAPQuery;
            static Common::Global<RAMessage> RAPQueryReply;

            static Common::Global<RAMessage> UpdateServiceDescription;
            static Common::Global<RAMessage> UpdateServiceDescriptionReply;

            Transport::ActionHeader actionHeader_;
        };
    }
}
