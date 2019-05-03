// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostContainer :
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
       DENY_COPY(ApplicationHostContainer)

    public:
        ~ApplicationHostContainer();

        static ApplicationHostContainer & GetApplicationHostContainer();
        ApplicationHostSPtr GetApplicationHost();

        Common::AsyncOperationSPtr BeginCreateComFabricRuntime(
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateComFabricRuntime(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<ComFabricRuntime> & comFabricRuntime);

        Common::AsyncOperationSPtr BeginRegisterCodePackageHost(
            Common::ComPointer<IFabricCodePackageHost> const & codePackageHost,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterCodePackageHost(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginGetComCodePackageActivationContext(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetComCodePackageActivationContext(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<ComCodePackageActivationContext> & comCodePackageActivationContextCPtr);

        Common::AsyncOperationSPtr BeginGetComCodePackageActivator(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetComCodePackageActivator(
            Common::AsyncOperationSPtr const & operation,
            _Out_ Common::ComPointer<ComApplicationHostCodePackageActivator> & comCodePackageActivatorCPtr);

        Common::AsyncOperationSPtr BeginGetFabricNodeContext(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetFabricNodeContext(
            Common::AsyncOperationSPtr const & operation,
            __out Common::ComPointer<ComFabricNodeContextResult> & comNodeContextCPtr);

        HRESULT CreateBackupRestoreAgent(
            REFIID riid,
            void **backupRestoreAgent);

    public:
         static /* [entry] */ HRESULT FabricBeginCreateRuntime(
        /* [in] */ __RPC__in REFIID riid,
        /* [in] */ __RPC__in_opt IFabricProcessExitHandler *exitHandler,
        /* [in] */ DWORD timeoutMilliseconds,
        /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

        static /* [entry] */ HRESULT FabricEndCreateRuntime( 
        /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
        /* [retval][out] */ __RPC__deref_out_opt void **fabricRuntime);

        static /* [entry] */ HRESULT FabricCreateRuntime( 
        /* [in] */ __RPC__in REFIID riid,
        /* [retval][out] */ __RPC__deref_out_opt void **fabricRuntime);

        static /* [entry] */ HRESULT FabricBeginGetActivationContext( 
        /* [in] */ __RPC__in REFIID riid,
        /* [in] */ DWORD timeoutMilliseconds,
        /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

        static /* [entry] */ HRESULT FabricEndGetActivationContext( 
        /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
        /* [retval][out] */ __RPC__deref_out_opt void **activationContext);

        static /* [entry] */ HRESULT FabricGetActivationContext( 
        /* [in] */ __RPC__in REFIID riid,
        /* [retval][out] */ __RPC__deref_out_opt void **activationContext);

        static /* [entry] */ HRESULT FabricRegisterCodePackageHost( 
        /* [in] */ __RPC__in_opt IUnknown *codePackageHost);

         static /* [entry] */ HRESULT FabricBeginGetNodeContext( 
        /* [in] */ DWORD timeoutMilliseconds,
        /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
        /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

        static /* [entry] */ HRESULT FabricEndGetNodeContext( 
        /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
        /* [retval][out] */ __RPC__deref_out_opt void ** nodeContext);

        static /* [entry] */ HRESULT FabricGetNodeContext( 
        /* [retval][out] */ __RPC__deref_out_opt void ** nodeContext);

        static /* [entry] */ HRESULT FabricCreateBackupRestoreAgent(
        /* [in] */ __RPC__in REFIID riid,
        /* [retval][out] */ __RPC__deref_out_opt void **backupRestoreAgent);

        static /* [entry] */ HRESULT FabricBeginGetCodePackageActivator(
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

        static /* [entry] */ HRESULT FabricEndGetCodePackageActivator(
            /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
            /* [retval][out] */ __RPC__deref_out_opt void **activator);

        static /* [entry] */ HRESULT FabricGetCodePackageActivator(
            /* [in] */ __RPC__in REFIID riid,
            /* [retval][out] */ __RPC__deref_out_opt void **activator);

        void Test_CloseApplicationHost();

    private:
        ApplicationHostContainer();

        // ensures a valid application host by either getting an already created and opened
        // application host or creates and open the application host
        Common::AsyncOperationSPtr BeginEnsureApplicationHost(
            ApplicationHostContext const & hostContext,
            Common::ComPointer<IFabricCodePackageHost> const & codePackageHost,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndEnsureApplicationHost(
            Common::AsyncOperationSPtr const & operation,
            __out ApplicationHostSPtr & host);

        Common::ErrorCode GetNodeContext(__out FabricNodeContextResultImplSPtr & result);
        void NodeHostProcessClosedEventHandler(NodeHostProcessClosedEventArgs const & eventArgs);

        bool ClearApplicationHost(std::wstring const & hostId);
        
        static Common::ErrorCode CreateApplicationHost(
            ApplicationHostContext const & hostContext,
            Common::ComPointer<IFabricCodePackageHost> const & codePackageHost,
            __out ApplicationHostSPtr & host);

        static Common::ErrorCode ApplicationHostContainer::GetRuntimeInformationFromEnv(
            Common::EnvironmentMap const & envMap,
            ApplicationHostContext const & hostContext,
            __out wstring & runtimeServiceAddress,
            __out Common::PCCertContext & certContextPtr,
            __out wstring & thumbprint);

        static Common::ErrorCode GetApplicationHostContext(
            __out ApplicationHostContext & hostContext);

        static Common::ErrorCode GetApplicationHostContext(
            Common::EnvironmentMap const & envMap,
            __out ApplicationHostContext & hostContext);

        static Common::ErrorCode GetCodePackageContext(
            Common::EnvironmentMap const & envMap,
            __out CodePackageContext & codeContext);

        static Common::ErrorCode StartTraceSessionInsideContainer();

    private:
        class CreateFabricRuntimeComAsyncOperationContext;
        friend class CreateFabricRuntimeComAsyncOperationContext;

        class CreateComFabricRuntimeAsyncOperation;
        friend class CreateComFabricRuntimeAsyncOperation;

        class GetActivationContextComAsyncOperationContext;
        friend class GetActivationContextComAsyncOperationContext;

        class GetCodePackageActivatorComAsyncOperationContext;
        friend class GetCodePackageActivatorComAsyncOperationContext;

        class GetComCodePackageActivationContextAsyncOperation;
        friend class GetComCodePackageActivationContextAsyncOperation;

        class GetComCodePackageActivatorAsyncOperation;
        friend class GetComCodePackageActivatorAsyncOperation;

        class RegisterCodePackageHostAsyncOperation;
        friend class RegisterCodePackageHostAsyncOperation;

        class EnsureApplicationHostAsyncOperation;
        friend class EnsureApplicationHostAsyncOperation;

        class OpenApplicationHostLinkedAsyncOperation;
        friend class OpenApplicationHostLinkedAsyncOperation;

        class GetNodeContextAsyncOperation;
        friend class GetNodeContextAsyncOperation;

        class GetNodeContextComAsyncOperationContext;
        friend class GetNodeContextComAsyncOperationContext;

    private:
        Common::RwLock lock_;
        ApplicationHostSPtr host_;
        Common::AsyncOperationSPtr pendingHostCreateOperation_;

        static ApplicationHostContainer * singleton_;
        static INIT_ONCE initOnceAppHostContainer_;
        static BOOL CALLBACK InitAppHostContainerFunction(PINIT_ONCE, PVOID, PVOID *);
    };
}
