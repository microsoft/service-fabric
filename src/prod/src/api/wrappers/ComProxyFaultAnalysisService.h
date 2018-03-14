// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyFaultAnalysisService :
		public ComProxySystemServiceBase<IFabricFaultAnalysisService>,
        public IFaultAnalysisService
    {
        DENY_COPY(ComProxyFaultAnalysisService);

    public:
        ComProxyFaultAnalysisService(Common::ComPointer<IFabricFaultAnalysisService> const & comImpl);
        virtual ~ComProxyFaultAnalysisService();

        virtual Common::AsyncOperationSPtr BeginStartPartitionDataLoss(
            FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndStartPartitionDataLoss(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginGetPartitionDataLossProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetPartitionDataLossProgress(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricPartitionDataLossProgressResult ** reply);

        virtual Common::AsyncOperationSPtr BeginStartPartitionQuorumLoss(
            FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndStartPartitionQuorumLoss(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginGetPartitionQuorumLossProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetPartitionQuorumLossProgress(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricPartitionQuorumLossProgressResult ** reply);

        virtual Common::AsyncOperationSPtr BeginStartPartitionRestart(
            FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndStartPartitionRestart(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginGetPartitionRestartProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetPartitionRestartProgress(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricPartitionRestartProgressResult ** reply);

        virtual Common::AsyncOperationSPtr BeginGetTestCommandStatusList(
            FABRIC_TEST_COMMAND_LIST_DESCRIPTION * description,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetTestCommandStatusList(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricTestCommandStatusResult ** reply);
        
        virtual Common::AsyncOperationSPtr BeginCancelTestCommand(
            FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION* description,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndCancelTestCommand(
            Common::AsyncOperationSPtr const & asyncOperation);
            
        virtual Common::AsyncOperationSPtr BeginStartChaos(
            FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndStartChaos(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginStopChaos(
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndStopChaos(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginGetChaosReport(
            FABRIC_GET_CHAOS_REPORT_DESCRIPTION * description,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndGetChaosReport(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricChaosReportResult ** reply);

        virtual Common::AsyncOperationSPtr ComProxyFaultAnalysisService::BeginGetStoppedNodeList(            
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode ComProxyFaultAnalysisService::EndGetStoppedNodeList(
            Common::AsyncOperationSPtr const & asyncOperation,            
            std::wstring & reply);

        virtual Common::AsyncOperationSPtr ComProxyFaultAnalysisService::BeginStartNodeTransition(
            FABRIC_NODE_TRANSITION_DESCRIPTION* description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode ComProxyFaultAnalysisService::EndStartNodeTransition(
            Common::AsyncOperationSPtr const & asyncOperation);

        virtual Common::AsyncOperationSPtr BeginGetNodeTransitionProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode ComProxyFaultAnalysisService::EndGetNodeTransitionProgress(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricNodeTransitionProgressResult ** reply);

		// SystemServiceCall
		virtual Common::AsyncOperationSPtr BeginCallSystemService(
			std::wstring const & action,
			std::wstring const & inputBlob,
			Common::TimeSpan const timeout,
			Common::AsyncCallback const & callback,
			Common::AsyncOperationSPtr const & parent);
		virtual Common::ErrorCode EndCallSystemService(
			Common::AsyncOperationSPtr const & asyncOperation,
			__inout std::wstring & outputBlob);

    private:
        class InvokeDataLossAsyncOperation;
        class GetInvokeDataLossProgressAsyncOperation;
        class InvokeQuorumLossAsyncOperation;
        class GetInvokeQuorumLossProgressAsyncOperation;
        class RestartPartitionAsyncOperation;
        class GetRestartPartitionProgressAsyncOperation;
        class GetTestCommandStatusAsyncOperation;
        class CancelTestCommandAsyncOperation;
        class GetStoppedNodeListAsyncOperation;
        class StartNodeTransitionAsyncOperation;
        class GetNodeTransitionProgressAsyncOperation;

        class StartChaosAsyncOperation;
        class StopChaosAsyncOperation;
        class GetChaosReportAsyncOperation;
    };
}
