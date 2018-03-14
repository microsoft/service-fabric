// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // REST Handler for FabricClient API's 
    //  * IFabricQueryClient - GetApplicationList
    //  * IFabricApplicationManagementClient 
    //
    class ApplicationTypesHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {

        DENY_COPY(ApplicationTypesHandler)

    public:

        ApplicationTypesHandler(HttpGatewayImpl & server);
        virtual ~ApplicationTypesHandler();

        Common::ErrorCode Initialize();

    private:

        void GetAllApplicationsTypes(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllApplicationTypesComplete(__in Common::AsyncOperationSPtr const & operation, __in bool expectedCompletedSynchronously);
        void OnGetApplicationTypesPagedComplete(__in Common::AsyncOperationSPtr const & operation, __in bool expectedCompletedSynchronously);
        void GetApplicationTypesPagedHelper(__in Common::AsyncOperationSPtr const& thisSPtr, HandlerAsyncOperation * const& handlerOperation, FabricClientWrapper const& client);

        void GetApplicationTypesByType(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetApplicationTypesByTypeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ProvisionApplicationType(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnProvisionComplete(bool isAsync, __in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UnprovisionApplicationType(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnUnprovisionComplete(bool isAsync, __in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetApplicationManifest(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetApplicationManifestComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceManifest(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetServiceManifestComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceTypes(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetServiceTypesComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceTypesByType(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetServiceTypesByTypeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        class ApplicationTypeVersionData : public Common::IFabricJsonSerializable
        {
            DENY_COPY_CONSTRUCTOR(ApplicationTypeVersionData)
            DENY_COPY_ASSIGNMENT(ApplicationTypeVersionData)

        public:
            ApplicationTypeVersionData() {};

            __declspec (property(get=get_AppTypeVersion)) std::wstring const& ApplicationTypeVersion;
            std::wstring const& get_AppTypeVersion() const { return appTypeVersion_; }

            __declspec(property(get=get_IsAsync)) bool IsAsync;
            bool get_IsAsync() const { return isAsync_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeVersion, appTypeVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Async, isAsync_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring appTypeVersion_;
            bool isAsync_;
        };

        class Manifest : public Common::IFabricJsonSerializable
        {
        public:
            Manifest(std::wstring&& manifest)
                :manifest_(move(manifest))
            {}

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Manifest, manifest_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring manifest_;
        };

    };
}
