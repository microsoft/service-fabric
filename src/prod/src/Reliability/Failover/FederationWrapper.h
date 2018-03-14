// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceResolver;


    class FederationWrapper : public Common::RootedObject, Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>, public IFederationWrapper
    {
        DENY_COPY(FederationWrapper);

    public:
        FederationWrapper(Federation::FederationSubsystem & federation, ServiceResolver * serviceResolver = nullptr);

        __declspec (property(get=get_NodeId)) Federation::NodeId Id;
        Federation::NodeId get_NodeId() const { return federation_.Id; }

        Federation::NodeInstance const& get_Instance() const { return federation_.Instance; }

        std::wstring const& get_InstanceIdStr() const { return instanceIdStr_; }

        bool get_IsShutdown() const { return federation_.IsShutdown; }
        
        __declspec (property(get=get_Federation)) Federation::FederationSubsystem & Federation;
        Federation::FederationSubsystem & get_Federation() { return federation_; }

        void SendToFM(Transport::MessageUPtr && message);
        void SendToFMM(Transport::MessageUPtr && message);
        void SendToNode(Transport::MessageUPtr && message, Federation::NodeInstance const & target);

        Common::AsyncOperationSPtr BeginRequestToFMM(
            Transport::MessageUPtr && message,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const;
        Common::ErrorCode EndRequestToFMM(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) const;

        virtual Common::AsyncOperationSPtr BeginRequestToFM(
            Transport::MessageUPtr && message,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode EndRequestToFM(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply);

        virtual Common::AsyncOperationSPtr BeginRequestToNode(
            Transport::MessageUPtr && message,
            Federation::NodeInstance const & target,
            bool useExactRouting,
            Common::TimeSpan const retryInterval,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) const;
        virtual Common::ErrorCode EndRequestToNode(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) const;

        void UpdateFMService(ServiceTableEntry const & fmEntry, GenerationNumber const & generation);

        void CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Transport::MessageUPtr && reply);
        void CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Common::ErrorCode const & error);

    private:
        void RouteCallback(
            Common::AsyncOperationSPtr const& routeOperation,
            std::wstring const & action,
            Federation::NodeInstance const & target);

        bool TryGetFMPrimary(Federation::NodeInstance & fmPrimaryInstance);
        void OnFMServiceResolved(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously);
        void InvalidateFMLocationIfNeeded(Common::ErrorCode const & error, Federation::NodeInstance const & target);

        static void SetFMMActor(Transport::Message & message);

        MUTABLE_RWLOCK(FM.FederationWrapper, lock_);

        Federation::FederationSubsystem & federation_;
        ServiceResolver * serviceResolver_;
        const std::wstring traceId_;
        int64 fmLookupVersion_;
        GenerationNumber fmGeneration_;
        Federation::NodeInstance fmPrimaryNodeInstance_;
        bool isFMPrimaryValid_;
        std::wstring instanceIdStr_;

        class FMRequestAsyncOperation;
    };
}
