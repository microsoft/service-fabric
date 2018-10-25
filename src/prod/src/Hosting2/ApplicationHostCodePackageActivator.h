// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostCodePackageActivator : 
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:
        ApplicationHostCodePackageActivator(
            Common::ComponentRoot const & root);

        virtual ~ApplicationHostCodePackageActivator();

        virtual Common::AsyncOperationSPtr BeginActivateCodePackage(
            std::vector<std::wstring> const & codePackageNames,
            Common::EnvironmentMap && environment,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndActivateCodePackage(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr BeginDeactivateCodePackage(
            std::vector<std::wstring> const & codePackageNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndDeactivateCodePackage(
            Common::AsyncOperationSPtr const & operation);

        virtual void AbortCodePackage(
            std::vector<std::wstring> const & codePackageNames);

        ULONGLONG RegisterCodePackageEventHandler(
            Common::ComPointer<IFabricCodePackageEventHandler> const & eventHandlerCPtr,
            std::wstring const & sourceId);

        virtual void UnregisterCodePackageEventHandler(ULONGLONG handlerId);

        void RegisterComCodePackageActivator(
            std::wstring const& sourceId,
            ITentativeApplicationHostCodePackageActivator const & comActivator);

        void UnregisterComCodePackageActivator(std::wstring const& sourceId);

        void NotifyEventHandlers(CodePackageEventDescription const & eventDescription);

    protected:
        virtual Common::AsyncOperationSPtr BeginSendRequest(
            ApplicationHostCodePackageOperationRequest && requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndSendRequest(
            Common::AsyncOperationSPtr const & operation,
            __out ApplicationHostCodePackageOperationReply & reply) = 0;

        virtual CodePackageContext GetCodePackageContext() = 0;
        virtual ApplicationHostContext GetHostContext() = 0;

    private:
        struct EventHandlerInfo;
        class ActivateAsyncOperation;
        class DeactivateAsyncOperation;

    private:
        void CopyEventHandlers(std::vector<std::shared_ptr<EventHandlerInfo>> & eventHandlersSnap);

        bool TryGetComSource(
            std::wstring const & sourceId, 
            _Out_ Common::ComPointer<ITentativeApplicationHostCodePackageActivator> & comSource);

        void TraceWarning(
            std::wstring && operationName,
            std::vector<std::wstring> && codePackageNames,
            Common::ErrorCode error);

    private:
        Common::RwLock eventHandlersLock_;
        std::vector<std::shared_ptr<EventHandlerInfo>> eventHandlers_;
        ULONGLONG handlerIdTracker_;
        std::map<std::wstring, Common::ReferencePointer<ITentativeApplicationHostCodePackageActivator>> comSourcePointers_;
    };
}
