// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{   
    class FaultsHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(FaultsHandler)

    public:

        FaultsHandler(HttpGatewayImpl & server);
        virtual ~FaultsHandler();

        Common::ErrorCode Initialize();

    private:

        void StartPartitionDataLossHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        void StartPartitionQuorumLossHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        void StartPartitionRestartHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        
        void OnStartPartitionDataLossComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously);
        void OnStartPartitionQuorumLossComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously);
        void OnStartPartitionRestartComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously);

        void GetDataLossProgressHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        void GetQuorumLossProgressHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        void GetPartitionRestartProgressHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        void GetNodeTransitionProgressHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        
        void GetProgress(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetProgressComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously,
            __in DWORD);

        void StartNodeTransitionHandler(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartNodeTransitionComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously);

        void GetTestCommandsList(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetTestCommandStatusListComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously);

        void CancelTestCommand(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancelTestCommandComplete(
            __in Common::AsyncOperationSPtr const& operation,
            __in bool expectedCompletedSynchronously);            

    class PartitionCommonData
    {
        DENY_COPY(PartitionCommonData)
                        
        public:
            PartitionCommonData(Common::Guid const& operationId, std::shared_ptr<Management::FaultAnalysisService::PartitionSelector> partitionSelector)
                : operationId_(operationId)
                , partitionSelector_(partitionSelector)
            {}
        
            __declspec(property(get=get_OperationId)) Common::Guid OperationId;
            Common::Guid get_OperationId() const { return operationId_; }                    
            
            __declspec(property(get=get_PartitionSelector)) std::shared_ptr<Management::FaultAnalysisService::PartitionSelector> const& PartitionSelector;
            std::shared_ptr<Management::FaultAnalysisService::PartitionSelector> const& get_PartitionSelector() const { return partitionSelector_; }
                    
            private:
                Common::Guid operationId_;            
                std::shared_ptr<Management::FaultAnalysisService::PartitionSelector> partitionSelector_;
        };

        static Common::ErrorCode TryGetCommonFields(
            __in UriArgumentParser & argumentParser,
            __out std::unique_ptr<PartitionCommonData> & partitionCommonData);

        Common::ErrorCode GetDataLossDescription(
            __in UriArgumentParser & argumentParser, 
            __out Management::FaultAnalysisService::InvokeDataLossDescription & description);
        Common::ErrorCode GetQuorumLossDescription(
            __in UriArgumentParser & argumentParser, 
            __out Management::FaultAnalysisService::InvokeQuorumLossDescription & description);
        Common::ErrorCode GetPartitionRestartDescription(
            __in UriArgumentParser & argumentParser, 
            __out Management::FaultAnalysisService::RestartPartitionDescription & description);
        Common::ErrorCode GetStartNodeTransitionDescription(
            __in UriArgumentParser & argumentParser, 
            __out Management::FaultAnalysisService::StartNodeTransitionDescription & description);
    };
}

