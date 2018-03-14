// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class MultiCodePackageApplicationHost :
        public ApplicationHost
    {
    public:
        MultiCodePackageApplicationHost(
            std::wstring const & hostId, 
            std::wstring const & runtimeServiceAddress,
            Common::PCCertContext certContext,
            wstring const & serverThumbprint,
            Common::ComPointer<IFabricCodePackageHost> codePackageHost);
        virtual ~MultiCodePackageApplicationHost();

        static Common::ErrorCode Create(
            std::wstring const & hostId, 
            std::wstring const & runtimeServiceAddress,
            Common::PCCertContext certContext,
            wstring const & serverThumbprint,
            Common::ComPointer<IFabricCodePackageHost> codePackageHost, 
            __out ApplicationHostSPtr & host);

    protected:

        virtual Common::ErrorCode OnCreateAndAddFabricRuntime(
            FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,            
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
            __out FabricRuntimeImplSPtr & fabricRuntime);

        virtual Common::ErrorCode OnGetCodePackageActivationContext(
            CodePackageContext const & codeContext,
            __out CodePackageActivationContextSPtr & codePackageActivationContext);

        virtual Common::ErrorCode OnUpdateCodePackageContext(
            CodePackageContext const & codeContext);

        virtual void ProcessIpcMessage(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);

    private:
        void ProcessActivateCodePackageRequest(
            Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void OnProcessActivateCodePackageRequestCompleted(
            Common::AsyncOperationSPtr const & operation);

        void FinishProcessActivateCodePackageRequest(
            Common::AsyncOperationSPtr const & operation);
        
        
        void ProcessDeactivateCodePackageRequest(
            Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);
        
        void OnProcessDeactivateCodePackageRequestCompleted(
            Common::AsyncOperationSPtr const & operation);

        void FinishProcessDeactivateCodePackageRequest(
            Common::AsyncOperationSPtr const & operation);
        

        void OnHostedCodePackageTerminated(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId);

        void OnCodePackageTerminationHandled(
            Common::AsyncOperationSPtr const & operation);

        void FinishCodePackageTerminationHandler(
            Common::AsyncOperationSPtr const & operation);
        

    private:

        class ActivateCodePackageRequestAsyncProcessor;
        friend class ActivateCodePackageRequestAsyncProcessor;

        class DeactivateCodePackageRequestAsyncProcessor;
        friend class DeactivateCodePackageRequestAsyncProcessor;

        class ComHostedCodePackageActivationContext;
        friend class ComHostedCodePackageActivationContext;

        class CodePackageTerminationAsyncHandler;
        friend class CodePackageTerminationAsyncHandler;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

    private:
        ComProxyCodePackageHost codePackageHostProxy_;
    };
}
