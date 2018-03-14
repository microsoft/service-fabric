// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class responsible with sending ACKs
        // either on timer or immediately, based on the configuration
        // settings.
        class AckSender 
        {
            DENY_COPY(AckSender)

        public:
            AckSender(
                REInternalSettingsSPtr const & config, 
                Common::Guid const & partitionId, 
                ReplicationEndpointId const & description);
                
            ~AckSender();

            __declspec (property(get=get_ForceSendAckOnMaxPendingItems)) bool ForceSendAckOnMaxPendingItems;
            bool get_ForceSendAckOnMaxPendingItems() const { return forceSendAckOnMaxPendingItems_; }

            __declspec (property(get = get_LastAckSentAt)) Common::DateTime LastAckSentAt;
            Common::DateTime get_LastAckSentAt() const;

            void Open(
                Common::ComponentRoot const & root,
                SendCallback const & sendCallback);
            void Close();

            void ScheduleOrSendAck(bool forceSend);
            
        private:

            void OnTimerCallback();

            bool ShouldTraceCallerHoldsLock();

            REInternalSettingsSPtr const config_;
            ReplicationEndpointId const endpointUniqueId_; 
            Common::Guid const partitionId_;
            SendCallback sendCallback_;
            bool isActive_;
            
            Common::Stopwatch lastTraceTime_;

            bool forceSendAckOnMaxPendingItems_;
            Common::DateTime lastAckSentAt_;

            // Timer for sending batched Acks
            Common::TimerSPtr batchSendTimer_;
            bool timerActive_;
            // Lock that protects the ACK batch timer.
            MUTABLE_RWLOCK(REAckSender, lock_);
        }; // end AckSender

    } // end namespace ReplicationComponent
} // end namespace Reliability
