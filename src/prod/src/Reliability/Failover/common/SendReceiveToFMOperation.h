// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class SendReceiveToFMOperation : 
        public std::enable_shared_from_this<SendReceiveToFMOperation>,
        public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>
    {
        DENY_COPY(SendReceiveToFMOperation);

    public:

        class DelayCalculator
        {
        public:
            DelayCalculator(
                Common::TimeSpan baseInterval, 
                double maxJitter);

            Common::TimeSpan GetDelay() const;

            __declspec(property(get = get_Test_Random)) Common::Random & Test_Random;
            Common::Random & get_Test_Random() { return random_; }

        private:
            Common::TimeSpan baseInterval_;
            Common::TimeSpan maxJitterInterval_;
            mutable Common::Random random_;
        };

        virtual ~SendReceiveToFMOperation();

        void Send();

        __declspec (property(get = get_IsCancelled, put = set_IsCancelled)) bool IsCancelled;
        bool get_IsCancelled() const { return isCancelled_; }
        void set_IsCancelled(bool const isCancelled) { isCancelled_ = isCancelled; }

    protected:
        Transport::MessageUPtr replyMessage_;
        IReliabilitySubsystem & reliabilitySubsystem_;

        __declspec (property(get=get_IsToFMM)) bool IsToFMM;
        bool get_IsToFMM() const { return isToFMM_; }

        virtual Transport::MessageUPtr CreateRequestMessage() const = 0;

        virtual void OnAckReceived();

        virtual Common::TimeSpan GetDelay() const;

        SendReceiveToFMOperation(
            IReliabilitySubsystem & reliability, 
            std::wstring const & ackAction, 
            bool isToFMM,
            DelayCalculator delayCalculator);

    private:
        std::wstring ackAction_;
        bool isToFMM_;
        bool isCancelled_;
        DelayCalculator delayCalculator_;

        void RouteRequestCallback(
            std::shared_ptr<SendReceiveToFMOperation> const& sendReceiveToFMOperation,
            Common::AsyncOperationSPtr const& routeRequestOperation);

        void RetryTimerCallback();
    };
}
