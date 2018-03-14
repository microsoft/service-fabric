// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // Provides a way to share runtime within fabric for various Fabric services
    class RuntimeSharingHelper :
        public Common::RootedObject,
        public IRuntimeSharingHelper,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(RuntimeSharingHelper)

    public:
        RuntimeSharingHelper(RuntimeSharingHelperConstructorParameters &);
        virtual ~RuntimeSharingHelper();

        Common::ComPointer<IFabricRuntime> GetRuntime();

        Transport::IpcClient & GetIpcClient();

        Common::AsyncOperationSPtr BeginOpen(
            std::wstring const & runtimeServiceAddress,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation);

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(            
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

    private:
        KtlSystem * ktlSystem_;
        std::wstring runtimeServiceAddress_;
        ApplicationHostSPtr host_;
        Common::ComPointer<ComFabricRuntime> runtime_;
    };
}
