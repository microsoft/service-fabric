// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IFaultAnalysisService
    {
    public:
        virtual ~IFaultAnalysisService() {};

		//// region: keep for backward compatibility purpose only

        // Invoke Data Loss
        virtual Common::AsyncOperationSPtr BeginStartPartitionDataLoss(
            FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION * invokeDataLossDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStartPartitionDataLoss(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPartitionDataLossProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetPartitionDataLossProgress(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricPartitionDataLossProgressResult ** result) = 0;

        // Invoke Quorum Loss
        virtual Common::AsyncOperationSPtr BeginStartPartitionQuorumLoss(
            FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION * invokeQuorumLossDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStartPartitionQuorumLoss(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPartitionQuorumLossProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetPartitionQuorumLossProgress(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricPartitionQuorumLossProgressResult ** result) = 0;

        // Restart Partition
        virtual Common::AsyncOperationSPtr BeginStartPartitionRestart(
            FABRIC_START_PARTITION_RESTART_DESCRIPTION * restartPartitionDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStartPartitionRestart(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPartitionRestartProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetPartitionRestartProgress(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricPartitionRestartProgressResult ** result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetTestCommandStatusList(
            FABRIC_TEST_COMMAND_LIST_DESCRIPTION * description,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetTestCommandStatusList(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricTestCommandStatusResult ** result) = 0;

        virtual Common::AsyncOperationSPtr BeginCancelTestCommand(
            FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION* descriptin,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndCancelTestCommand(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        // Chaos
        virtual Common::AsyncOperationSPtr BeginStartChaos(
            FABRIC_START_CHAOS_DESCRIPTION * startChaosDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStartChaos(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginStopChaos(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndStopChaos(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetChaosReport(
            FABRIC_GET_CHAOS_REPORT_DESCRIPTION * description,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetChaosReport(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricChaosReportResult ** result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetStoppedNodeList(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetStoppedNodeList(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out std::wstring & reply) = 0;

        virtual Common::AsyncOperationSPtr BeginStartNodeTransition(
            FABRIC_NODE_TRANSITION_DESCRIPTION* description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndStartNodeTransition(
            Common::AsyncOperationSPtr const & asyncOperation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeTransitionProgress(
            FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetNodeTransitionProgress(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricNodeTransitionProgressResult ** reply) = 0;

		//// endregion

		// SystemServiceCall
		virtual Common::AsyncOperationSPtr BeginCallSystemService(
			std::wstring const & action,
			std::wstring const & inputBlob,
			Common::TimeSpan const timeout,
			Common::AsyncCallback const & callback,
			Common::AsyncOperationSPtr const & parent) = 0;
		virtual Common::ErrorCode EndCallSystemService(
			Common::AsyncOperationSPtr const & asyncOperation,
			__inout std::wstring & outputBlob) = 0;
    };
}
