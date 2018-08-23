//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class ApplicationsResourceHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ApplicationsResourceHandler)

    public:

        ApplicationsResourceHandler(HttpGatewayImpl & server);
        virtual ~ApplicationsResourceHandler();

        Common::ErrorCode Initialize();

    private:

        void CreateOrUpdateApplication(Common::AsyncOperationSPtr const &);
        void OnCreateOrUpdateApplicationComplete(Common::AsyncOperationSPtr const &, bool);

        void DeleteApplication(Common::AsyncOperationSPtr const &);
        void OnDeleteApplicationComplete(Common::AsyncOperationSPtr const &, bool);

        void GetAllApplications(Common::AsyncOperationSPtr const &);
        void OnGetAllApplicationsComplete(Common::AsyncOperationSPtr const &, bool);

        void GetApplicationByName(Common::AsyncOperationSPtr const &);
        void OnGetApplicationByNameComplete(Common::AsyncOperationSPtr const &, bool);

        void GetAllServices(Common::AsyncOperationSPtr const &);
        void OnGetAllServicesComplete(Common::AsyncOperationSPtr const &, bool);

        void GetServiceByName(Common::AsyncOperationSPtr const &);
        void OnGetServiceByNameComplete(Common::AsyncOperationSPtr const &, bool);

        void GetAllReplicas(Common::AsyncOperationSPtr const &);
        void OnGetAllReplicasComplete(Common::AsyncOperationSPtr const &, bool);

        void GetReplicaByName(Common::AsyncOperationSPtr const &);
        void OnGetReplicaByNameComplete(Common::AsyncOperationSPtr const &, bool);

        void GetContainerCodePackageLogs(Common::AsyncOperationSPtr const &);
        void OnGetContainerCodePackageLogsComplete(Common::AsyncOperationSPtr const &, bool);
    };
}
