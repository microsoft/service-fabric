// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::ClientAsyncOperationBase 
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>        
    {
    public:
        ClientAsyncOperationBase(
            __in FabricClientImpl &,
            Transport::FabricActivityHeader &&,
            Common::ErrorCode && passThroughError,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        __declspec(property(get=get_Client)) FabricClientImpl & Client;
        FabricClientImpl & get_Client() const { return client_; }

        __declspec(property(get=get_ClientEventSource)) ClientEventSource const & Trace; 
        ClientEventSource const & get_ClientEventSource() const { return client_.Trace; }

        __declspec(property(get=get_TraceContext)) std::wstring const & TraceContext;
        std::wstring const & get_TraceContext() const { return client_.TraceContext; }

        __declspec(property(get=get_ActivityHeader)) Transport::FabricActivityHeader const & ActivityHeader;
        Transport::FabricActivityHeader const & get_ActivityHeader() const { return activityHeader_; }

        __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
        Common::ActivityId const & get_ActivityId() const { return activityHeader_.ActivityId; }

        __declspec(property(get=get_RemainingTime)) Common::TimeSpan RemainingTime;
        Common::TimeSpan get_RemainingTime() const { return timeoutHelper_.GetRemainingTime(); }

    protected:
        void OnStart(Common::AsyncOperationSPtr const &);

        virtual void OnStartOperation(Common::AsyncOperationSPtr const &) = 0;

    private:
        FabricClientImpl & client_;
        Transport::FabricActivityHeader activityHeader_;
        Common::ErrorCode passThroughError_;
        Common::TimeoutHelper timeoutHelper_;
    };
}
