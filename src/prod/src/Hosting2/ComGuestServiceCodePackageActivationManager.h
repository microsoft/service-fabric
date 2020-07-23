// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    typedef std::map<
        std::wstring, 
        std::pair<bool, int64>,
        Common::IsLessCaseInsensitiveComparer<std::wstring>> CodePackageEventTracker;

    class ComGuestServiceCodePackageActivationManager :
        public IFabricCodePackageEventHandler,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComGuestServiceCodePackageActivationManager)

        COM_INTERFACE_LIST1(
            ComGuestServiceCodePackageActivationManager,
            IID_IFabricCodePackageEventHandler,
            IFabricCodePackageEventHandler)

    public:
        ComGuestServiceCodePackageActivationManager(IGuestServiceTypeHost & typeHost);

        virtual ~ComGuestServiceCodePackageActivationManager();

        Common::ErrorCode Open();
        void Close();

        Common::AsyncOperationSPtr BeginActivateCodePackagesAndWaitForEvent(
            Common::EnvironmentMap && environment,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndActivateCodePackagesAndWaitForEvent(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeactivateCodePackages(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDeactivateCodePackages(
            Common::AsyncOperationSPtr const & operation);

        void AbortCodePackages();

        Common::ErrorCode RegisterServiceInstance(
            Common::ComPointer<IGuestServiceInstance> const & serviceInstance,
            _Out_ ULONGLONG & registrationHandle);
        Common::ErrorCode RegisterServiceReplica(
            Common::ComPointer<IGuestServiceReplica> const & serviceReplica,
            _Out_ ULONGLONG & registrationHandle);

        void UnregisterServiceReplicaOrInstance(ULONGLONG serviceRegistrationhandle);

        void Test_GetCodePackageEventTracker(CodePackageEventTracker & eventTracker);

    public:
        HRESULT STDMETHODCALLTYPE BeginActivateCodePackagesAndWaitForEvent(
            /* [in] */ FABRIC_STRING_MAP *environment,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndActivateCodePackagesAndWaitForEvent(
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeactivateCodePackages(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeactivateCodePackages(
            /* [in] */ IFabricAsyncOperationContext *context);

    public:
        void STDMETHODCALLTYPE OnCodePackageEvent(
            /* [in] */ IFabricCodePackageActivator *source,
            /* [in] */ const FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION *eventDesc) override;

    private:
        void NotifyServices(CodePackageEventDescription && eventDesc);
        void TrackCodePackage(CodePackageEventDescription && eventDesc);
        Common::TimeSpan GetTimeout(DWORD timeoutMilliseconds);
        bool IsFailureCountExceeded(CodePackageEventDescription const & eventDesc);

    private:
        class ActivateCodePackagesAsyncOperation;
        class DeactivateCodePackagesAsyncOperation;
        class EventWaitAsyncOperation;
        class ActivateCodePackagesAndWaitForEventAsyncOperation;

        class ActivateCodePackagesComAsyncOperationContext;
        class DeactivateCodePackagesComAsyncOperationContext;

    private:
        IGuestServiceTypeHost & typeHost_;
        Common::atomic_bool isClosed_;

        ULONGLONG eventHandlerHanlde_;
        Common::ComPointer<IFabricCodePackageActivator> activator_;

        Common::RwLock serviceRegistrationLock_;
        ULONGLONG serviceRegistrationIdTracker_;
        std::map<ULONGLONG, Common::ComPointer<IGuestServiceInstance>> registeredServiceInstanceMap_;
        std::map<ULONGLONG, Common::ComPointer<IGuestServiceReplica>> registeredServiceReplicaMap_;

        int eventReceivedCount_;
        Common::RwLock eventLock_;
        CodePackageEventTracker codePackageEventTracker_;
        std::map<std::wstring, Common::AsyncOperationSPtr> eventWaiters_;
    };
}
