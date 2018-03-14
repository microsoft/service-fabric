// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::ForwardToServiceAsyncOperation : public FabricClientImpl::ClientAsyncOperationBase
    {
    public:
        ForwardToServiceAsyncOperation(
            __in FabricClientImpl & client,
            ClientServerRequestMessageUPtr && message,
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            __in_opt Common::ErrorCode && = Common::ErrorCode::Success());

        __declspec(property(get=get_Action)) std::wstring const & Action;
        std::wstring const & get_Action() const { return action_; }

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        Common::NamingUri const & get_Name() const { return name_; }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const &, __out ClientServerReplyMessageUPtr &);
        static Common::ErrorCode End(Common::AsyncOperationSPtr const &, __out ClientServerReplyMessageUPtr &, __out Common::ActivityId & activityId);

    protected:
        void OnStartOperation(Common::AsyncOperationSPtr const &);

    private:
        static Common::ErrorCode InternalEnd(Common::AsyncOperationSPtr const &, __out ClientServerReplyMessageUPtr &, __out Common::ActivityId & activityId);
        void OnForwardToServiceComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        ClientServerRequestMessageUPtr message_;
        std::wstring action_;
        Common::NamingUri name_;
        ClientServerReplyMessageUPtr reply_;
        Common::ActivityId activityId_;
    };
}
