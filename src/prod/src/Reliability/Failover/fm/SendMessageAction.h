// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        template <class T>
        class SendMessageAction : public StateMachineAction
        {
            DENY_COPY(SendMessageAction);

        public:

            SendMessageAction(
                RSMessage const& message,
                Federation::NodeInstance const& target,
                T && body);

            int OnPerformAction(FailoverManager & failoverManager);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const& option) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        protected:

            __declspec (property(get=get_Message)) RSMessage const& Message;
            RSMessage const& get_Message() const { return message_; }

            __declspec (property(get=get_Body)) T const& Body;
            T const& get_Body() const { return body_; }
            
            __declspec (property(get=get_Target)) Federation::NodeInstance const& Target;
            Federation::NodeInstance const& get_Target() const { return target_; }

        private:

            void WriteBody(Common::TextWriter & w) const;

            RSMessage const& message_;
            Federation::NodeInstance target_;
            T body_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class AddInstanceAction : public SendMessageAction<ReplicaMessageBody>
        {
            DENY_COPY(AddInstanceAction);

        public:

            AddInstanceAction(Replica const& replica);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class RemoveInstanceAction : public SendMessageAction<DeleteReplicaMessageBody>
        {
            DENY_COPY(RemoveInstanceAction);

        public:
            
            RemoveInstanceAction(Replica const& replica);
            RemoveInstanceAction(Federation::NodeInstance const& target, DeleteReplicaMessageBody && body);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class DoReconfigurationAction : public SendMessageAction<DoReconfigurationMessageBody>
        {
            DENY_COPY(DoReconfigurationAction);

        public:

            DoReconfigurationAction(FailoverUnit const& failoverUnit);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class AddPrimaryAction : public SendMessageAction<ReplicaMessageBody>
        {
            DENY_COPY(AddPrimaryAction);

        public:

            AddPrimaryAction(FailoverUnit const& failoverUnit);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class AddReplicaAction : public SendMessageAction<ReplicaMessageBody>
        {
            DENY_COPY(AddReplicaAction);

        public:

            AddReplicaAction(FailoverUnit const& failoverUnit, Replica const& replica);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class RemoveReplicaAction : public SendMessageAction<ReplicaMessageBody>
        {
            DENY_COPY(RemoveReplicaAction);

        public:

            RemoveReplicaAction(Replica  & replica, Federation::NodeInstance const& target);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class DropReplicaAction : public SendMessageAction<ReplicaMessageBody>
        {
            DENY_COPY(DropReplicaAction);

        public:

            DropReplicaAction(Replica const& replica);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class DeleteReplicaAction : public SendMessageAction<DeleteReplicaMessageBody>
        {
            DENY_COPY(DeleteReplicaAction);

        public:

            DeleteReplicaAction(Replica const& replica);
            DeleteReplicaAction(Federation::NodeInstance const& target, Replica const& replica);
            DeleteReplicaAction(Federation::NodeInstance const& target, DeleteReplicaMessageBody && body);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class ReplicaDroppedReplyAction : public SendMessageAction<ReplicaReplyMessageBody>
        {
            DENY_COPY(ReplicaDroppedReplyAction);

        public:

            ReplicaDroppedReplyAction(ReplicaReplyMessageBody && body, Federation::NodeInstance const& from);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class ChangeConfigurationReplyAction : public SendMessageAction<FailoverUnitReplyMessageBody>
        {
            DENY_COPY(ChangeConfigurationReplyAction);

        public:

            ChangeConfigurationReplyAction(FailoverUnit const& failoverUnit, Federation::NodeInstance const& from);
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class ReplicaEndpointUpdatedReplyAction : public SendMessageAction<ReplicaReplyMessageBody>
        {
            DENY_COPY(ReplicaEndpointUpdatedReplyAction);

        public:

            ReplicaEndpointUpdatedReplyAction(ReplicaReplyMessageBody && body, Federation::NodeInstance const& from);
        };
    }
}
