// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // downloads application and service package from imagestore
    class HostingQueryManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(HostingQueryManager)

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    public:
        HostingQueryManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);

        virtual ~HostingQueryManager();

        void InitializeLegacyTestabilityRequestHandler(
            LegacyTestabilityRequestHandlerSPtr const & legacyTestabilityRequestHandler)
        {
            legacyTestabilityRequestHandlerSPtr_ = legacyTestabilityRequestHandler;
        }

        Common::AsyncOperationSPtr BeginProcessIncomingQueryMessage(
            Transport::Message & message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessIncomingQueryMessage(
            Common::AsyncOperationSPtr const & asyncOperation,
            _Out_ Transport::MessageUPtr & reply);

        // Query implementations
        Common::ErrorCode GetApplicationListDeployedOnNode(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode GetApplicationPagedListDeployedOnNode(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode GetServiceManifestListDeployedOnNode(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode GetServiceTypeListDeployedOnNode(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode GetCodePackageListDeployedOnNode(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring applicationNameArgument,
            std::wstring serviceManifestNameArgument,
            std::wstring codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring const & applicationNameArgument,
            std::wstring const & serviceManifestNameArgument,
            std::wstring const & servicePackageActivationId,
            std::wstring const & codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndRestartDeployedPackage(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr& reply);

    private:

        Common::ErrorCode GetApplicationEntry(
            std::wstring const & filterApplicationName,
            Common::ActivityId const & activityId,
            __out Application2SPtr & applicationEntry);

        Common::ErrorCode GetServicePackages(
            Application2SPtr const & applicationEntry,
            std::wstring const & filterServiceManifestName,
            Common::ActivityId const & activityId,
            __out std::vector<ServicePackage2SPtr> & servicePackages);

        void TraceDeployedApplicationQueryResult(
            Common::ActivityId const & activityId,
            std::wstring const & filterApplicationName,
            std::map<std::wstring, ServiceModel::DeployedApplicationQueryResult> const & deployedApplicationQueryResult);

        void TraceDeployedServiceManifestQueryResult(
            Common::ActivityId const & activityId,
            std::wstring const & filterApplicationName,
            std::wstring const & filterServiceManifestName,
            std::map<std::wstring, ServiceModel::DeployedServiceManifestQueryResult> const & deployedServiceManifestQueryResult);

        inline Common::ErrorCode ReplaceErrorIf(
            Common::ErrorCode actualError,
            Common::ErrorCodeValue::Enum compareWithError,
            Common::ErrorCodeValue::Enum changeToError);

        Common::ErrorCode GetCodePackage(
            int64 codePackageInstanceId,
            std::wstring const & applicationNameArgument,
            std::wstring const & serviceManifestNameArgument,
            std::wstring const & servicePackageActivationId,
            std::wstring const & codePackageNameArgument,
            Common::ActivityId const activityId,
            __out CodePackageSPtr & matchingCodePackage);
        
        // If a non empty continuation token is provided, then don't return DeployedApplicationQueryResults which do not comply with the continuation token
        Common::ErrorCode GetAggregatedApplicationsDeployedOnNode(
            std::wstring const & applicationNameArgument,
            ActivityId const & activityId,
            std::wstring const & continuationToken,
            __out map<wstring, ServiceModel::DeployedApplicationQueryResult> & deployedApplicationsMap);

        template<class T>
        std::vector<T> GetValues(std::map<std::wstring, T> const & deployedMap)
        {
            vector<T> values;
            transform(
                deployedMap.begin(),
                deployedMap.end(),
                back_inserter(values),
                [](typename map<std::wstring, T>::value_type const & val) { return val.second; });

            return values;
        }

        typedef Common::DefaultJobItem < HostingQueryManager > QueryJobItem;
        class QueryJobQueue : public Common::JobQueue<QueryJobItem, HostingQueryManager>
        {
        public:
            QueryJobQueue(std::wstring const & name, HostingQueryManager & root, bool forceEnqueue = false, int maxThreads = 0)
                : JobQueue(name, root, forceEnqueue, maxThreads)
            {
            }
        };

    private:

        HostingSubsystem & hosting_;
        Query::QueryMessageHandlerUPtr queryMessageHandler_;
        std::unique_ptr<QueryJobQueue> queryJobQueue_;
        LegacyTestabilityRequestHandlerSPtr legacyTestabilityRequestHandlerSPtr_;

        class ProcessQueryAsyncOperation;
        class RestartDeployedCodePackageAsyncOperation;

        Common::AsyncOperationSPtr BeginProcessQuery(
            Query::QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndProcessQuery(
            Common::AsyncOperationSPtr const & operation,
            _Out_ Transport::MessageUPtr & reply);
    };
}
