// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::Test_TestNamespaceManagerAsyncOperation : public Common::AsyncOperation
    {
    public:
        Test_TestNamespaceManagerAsyncOperation(
            __in FabricClientImpl & owner,
            std::wstring const & svcName,
            size_t byteCount,
            Transport::ISendTarget::SPtr const & directTarget,
            SystemServices::SystemServiceLocation const & primaryLocation,
            bool gatewayProxy,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out std::vector<byte> & result);

    protected:

        void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:

        void OnDirectSendCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void OnRequestCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void OnForwardCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void ProcessReply(Common::AsyncOperationSPtr const & thisSPtr, __in ClientServerReplyMessageUPtr & reply, __in Common::ErrorCode & error);

        void AddServiceTargetHeader(__in ClientServerRequestMessage & message);

        FabricClientImpl & owner_;
        std::wstring svcName_;
        Common::NamingUri svcUri_;
        size_t byteCount_;
        Transport::ISendTarget::SPtr directTarget_;
        SystemServices::SystemServiceLocation primaryLocation_;
        bool gatewayProxy_;
        Common::TimeoutHelper timeoutHelper_;
        std::vector<byte> result_;
    };
}
