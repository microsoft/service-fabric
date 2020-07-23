// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestCommands
    {
    public:
        static std::wstring const AddCommand;
        static std::wstring const RemoveCommand;
        static std::wstring const AbortCommand;
        static std::wstring const RemoveDataParameter;

        static std::wstring const ClearTicketCommand;
        static std::wstring const ResetStoreCommand;

        //This command calls ResetStore and ClearTicket with no params
        static std::wstring const CleanTestCommand;
        static std::wstring const SetPropertyCommand;
        static std::wstring const DefineFailoverManagerServiceCommand;
        static std::wstring const DefineNamingServiceCommand;
        static std::wstring const DefineClusterManagerServiceCommand;
        static std::wstring const DefineImageStoreServiceCommand;
        static std::wstring const DefineInfrastructureServiceCommand;
        static std::wstring const RemoveInfrastructureServiceCommand;
        static std::wstring const DefineRepairManagerServiceCommand;
        static std::wstring const DefineDnsServiceCommand;
        static std::wstring const DefineNIMServiceCommand;
        static std::wstring const DefineEnableUnsupportedPreviewFeaturesCommand;
        static std::wstring const DefineMockImageBuilderCommand;
        static std::wstring const ListCommand;
        static std::wstring const FMCommand;
        static std::wstring const UpdateServicePackageVersionInstanceCommand;
        static std::wstring const ShowNodesCommand;
        static std::wstring const ShowGFUMCommand;
        static std::wstring const ShowLFUMCommand;
        static std::wstring const ShowActiveServicesCommand;
        static std::wstring const ShowLoadMetricInfoCommand;
        static std::wstring const VerifyCommand;
        static std::wstring const VerifyFromFMCommand;
        static std::wstring const QueryCommand;
        static std::wstring const ExecuteQueryCommand;
        static std::wstring const ReportHealthCommand;
        static std::wstring const ReportHealthIpcCommand;
        static std::wstring const ReportHealthInternalCommand;
        static std::wstring const DeleteHealthCommand;
        static std::wstring const QueryHealthCommand;
        static std::wstring const QueryHealthListCommand;
        static std::wstring const QueryHealthStateChunkCommand;
        static std::wstring const CheckHMEntityCommand;
        static std::wstring const CorruptHMEntityCommand;
        static std::wstring const CheckHealthClientCommand;
        static std::wstring const CheckHMCommand;
        static std::wstring const SetHMThrottleCommand;
        static std::wstring const TakeClusterHealthSnapshotCommand;
        static std::wstring const CheckIsClusterHealthyCommand;
        static std::wstring const HealthPreInitializeCommand;
        static std::wstring const HealthPostInitializeCommand;
        static std::wstring const HealthGetProgressCommand;
        static std::wstring const HealthSkipSequenceCommand;
        static std::wstring const ResetHealthClientCommand;
        static std::wstring const WatchDogCommand;
        static std::wstring const HMLoadTestCommand;
        static std::wstring const SetFabricClientSettingsCommand;
        static std::wstring const AddRuntimeCommand;
        static std::wstring const RemoveRuntimeCommand;
        static std::wstring const UnregisterRuntimeCommand;
        static std::wstring const RegisterServiceTypeCommand;
        static std::wstring const EnableServiceTypeCommand;
        static std::wstring const DisableServiceTypeCommand;
        static std::wstring const InfrastructureCommand;
        static std::wstring const CreateRepairCommand;
        static std::wstring const CancelRepairCommand;
        static std::wstring const ForceApproveRepairCommand;
        static std::wstring const DeleteRepairCommand;
        static std::wstring const UpdateRepairCommand;
        static std::wstring const ShowRepairsCommand;
        static std::wstring const UpdateRepairHealthPolicyCommand;
        static std::wstring const GetRepairCommand;

        static std::wstring const CreateNameCommand;
        static std::wstring const DeleteNameCommand;
        static std::wstring const DnsNameExistsCommand;
        static std::wstring const NameExistsCommand;
        static std::wstring const PutPropertyCommand;
        static std::wstring const PutCustomPropertyCommand;
        static std::wstring const SubmitPropertyBatchCommand;
        static std::wstring const DeletePropertyCommand;
        static std::wstring const GetPropertyCommand;
        static std::wstring const GetMetadataCommand;
        static std::wstring const EnumerateNamesCommand;
        static std::wstring const EnumeratePropertiesCommand;
        static std::wstring const StartTestFabricClientCommand;
        static std::wstring const StopTestFabricClientCommand;
        static std::wstring const VerifyPropertyEnumerationCommand;

        static std::wstring const CreateServiceGroupCommand;
        static std::wstring const UpdateServiceGroupCommand;
        static std::wstring const DeleteServiceGroupCommand;
        static std::wstring const InjectFailureCommand;
        static std::wstring const RemoveFailureCommand;
        static std::wstring const SetSignalCommand;
        static std::wstring const ResetSignalCommand;
        static std::wstring const WaitForSignalHitCommand;
        static std::wstring const FailIfSignalHitCommand;
        static std::wstring const WaitAsyncCommand;

        static std::wstring const CreateServiceCommand;
        static std::wstring const UpdateServiceCommand;
        static std::wstring const CreateServiceFromTemplateCommand;
        static std::wstring const DeleteServiceCommand;
        static std::wstring const ResolveServiceCommand;
        static std::wstring const ResolveServiceWithEventsCommand;
        static std::wstring const PrefixResolveServiceCommand;
        static std::wstring const CreateServiceNotificationClient;
        static std::wstring const RegisterServiceNotificationFilter;
        static std::wstring const UnregisterServiceNotificationFilter;
        static std::wstring const SetServiceNotificationWait;
        static std::wstring const ServiceNotificationWait;
        static std::wstring const GetServiceDescriptionCommand;
        static std::wstring const GetServiceDescriptionUsingClientCommand;
        static std::wstring const GetServiceGroupDescriptionCommand;

        static std::wstring const DeactivateNodeCommand;
        static std::wstring const ActivateNodeCommand;

        static std::wstring const DeactivateNodesCommand;
        static std::wstring const RemoveNodeDeactivationCommand;
        static std::wstring const VerifyNodeDeactivationStatusCommand;
        static std::wstring const UpdateNodeImagesCommand;

        static std::wstring const NodeStateRemovedCommand;
        static std::wstring const RecoverPartitionsCommand;
        static std::wstring const RecoverPartitionCommand;
        static std::wstring const RecoverServicePartitionsCommand;
        static std::wstring const RecoverSystemPartitionsCommand;
        static std::wstring const ClientReportFaultCommand;
        static std::wstring const ResetPartitionLoadCommand;
        static std::wstring const ToggleVerboseServicePlacementHealthReportingCommand;

        static std::wstring const ParallelProvisionApplicationTypeCommand;
        static std::wstring const ProvisionApplicationTypeCommand;
        static std::wstring const UnprovisionApplicationTypeCommand;
        static std::wstring const CreateApplicationCommand;
        static std::wstring const UpdateApplicationCommand;
        static std::wstring const DeleteApplicationCommand;
        static std::wstring const UpgradeApplicationCommand;
        static std::wstring const UpdateApplicationUpgradeCommand;
        static std::wstring const VerifyApplicationUpgradeDescriptionCommand;
        static std::wstring const RollbackApplicationUpgradeCommand;
        static std::wstring const SetRollbackApplicationCommand;
        static std::wstring const UpgradeAppStatusCommand;
        static std::wstring const UpgradeAppMoveNext;
        static std::wstring const VerifyUpgradeAppCommand;
        static std::wstring const EnableNativeImageStore;
        static std::wstring const VerifyImageStore;
        static std::wstring const VerifyNodeFiles;
        static std::wstring const DeployServicePackageCommand;
        static std::wstring const VerifyDeployedCodePackageCountCommand;

        static std::wstring const CreateComposeCommand;
        static std::wstring const DeleteComposeCommand;
        static std::wstring const UpgradeComposeCommand;
        static std::wstring const RollbackComposeCommand;

        // For container network related tests
        static std::wstring const CreateNetworkCommand;
        static std::wstring const DeleteNetworkCommand;
        static std::wstring const GetNetworkCommand;
        static std::wstring const ShowNetworksCommand;

        static std::wstring const PrepareUpgradeFabricCommand;
        static std::wstring const ProvisionFabricCommand;
        static std::wstring const UnprovisionFabricCommand;
        static std::wstring const UpgradeFabricCommand;
        static std::wstring const UpdateFabricUpgradeCommand;
        static std::wstring const VerifyFabricUpgradeDescriptionCommand;
        static std::wstring const RollbackFabricUpgradeCommand;
        static std::wstring const SetRollbackFabricCommand;
        static std::wstring const UpgradeFabricStatusCommand;
        static std::wstring const UpgradeFabricMoveNext;
        static std::wstring const VerifyUpgradeFabricCommand;

        static std::wstring const FailFabricUpgradeDownloadCommand;
        static std::wstring const FailFabricUpgradeValidationCommand;
        static std::wstring const FailFabricUpgradeCommand;

        static std::wstring const ProcessPendingPlbUpdates;
        static std::wstring const SwapPrimaryCommand;
        static std::wstring const PromoteToPrimaryCommand;
        static std::wstring const MoveSecondaryCommand;
        static std::wstring const MoveSecondaryFromClientCommand;
        static std::wstring const MovePrimaryFromClientCommand;
        static std::wstring const DropReplicaCommand;
        static std::wstring const PLBUpdateService;

        // For support of Disk Drives config
        static std::wstring const AddDiskDriveFoldersCommand;
        static std::wstring const VerifyDeletedDiskDriveFoldersCommand;

        //if containers are not available just bail
        static std::wstring const CheckContainersAvailable;

        // for service location change notification
        static std::wstring const CreateClientCommand;
        static std::wstring const DeleteClientCommand;
        static std::wstring const RegisterCallbackCommand;
        static std::wstring const UnregisterCallbackCommand;
        static std::wstring const WaitForCallbackCommand;
        static std::wstring const VerifyCachedServiceResolutionCommand;
        static std::wstring const RemoveCachedServiceResolutionCommand;
        static std::wstring const ResolveUsingClientCommand;

        static std::wstring const Repro789586Command;
        static std::wstring const PerformanceTestCommand;

        static std::wstring const ClientPutCommand;
        static std::wstring const ClientGetCommand;
        static std::wstring const ClientGetAllCommand;
        static std::wstring const ClientGetKeysCommand;
        static std::wstring const ClientDeleteCommand;
        static std::wstring const ClientBackup;
        static std::wstring const ClientRestore;
        static std::wstring const ClientCompressionTest;
        static std::wstring const VerifyFabricTimeCommand;

        static std::wstring const ReportFaultCommand;
        static std::wstring const ReportLoadCommand;
        static std::wstring const ReportMoveCostCommand;

        static std::wstring const VerifyLoadReportCommand;
        static std::wstring const VerifyLoadValueCommand;
        static std::wstring const VerifyMoveCostValueCommand;
        static std::wstring const VerifyNodeLoadCommand;
        static std::wstring const VerifyPartitionLoadCommand;
        static std::wstring const VerifyClusterLoadCommand;
        static std::wstring const VerifyUnplacedReasonCommand;
        static std::wstring const VerifyApplicationLoadCommand;
        static std::wstring const VerifyResourceOnNodeCommand;
        static std::wstring const VerifyLimitsEnforcedCommand;
        static std::wstring const VerifyPLBAndLRMSync;
        static std::wstring const VerifyContainerPodsCommand;

        static std::wstring const VerifyReadWriteStatusCommand;

        static std::wstring const CheckIfLFUPMIsEmptyCommand;

        static std::wstring const StartClientCommand;
        static std::wstring const StopClientCommand;
        static std::wstring const UseBackwardsCompatibleClientsCommand;

        static std::wstring const SetSecondaryPumpEnabledCommand;

        // Unreliable Transport
        static std::wstring const AddBehaviorCommand;
        static std::wstring const RemoveBehaviorCommand;
        static std::wstring const CheckUnreliableTransportIsDisabledCommand;
        static std::wstring const CheckTransportBehaviorlist;
        static std::wstring const AddLeaseBehaviorCommand;
        static std::wstring const RemoveLeaseBehaviorCommand;

        // FabricTestHost Commands
        static std::wstring const GetPlacementDataCommand;
        static std::wstring const KillCodePackageCommand;
        static std::wstring const PutDataCommand;
        static std::wstring const GetDataCommand;
        static std::wstring const FailFastCommand;
        static std::wstring const SetCodePackageKillPendingCommand;

        // For testing dataloss scenarios
        static std::wstring const KillServiceCommand;
        static std::wstring const KillFailoverManagerServiceCommand;
        static std::wstring const KillNamingServiceCommand;

        // For testing fault scenarios
        static std::wstring const AddServiceBehaviorCommand;
        static std::wstring const RemoveServiceBehaviorCommand;

        static std::wstring const EnableTransportThrottleCommand;
        static std::wstring const ReloadTransportThrottleCommand;

        static std::wstring const DebugBreakCommand;
        static std::wstring const InjectErrorCommand;
        static std::wstring const RemoveErrorCommand;

        static std::wstring const CallService;

        static std::wstring const MakeDropCommand;

        // Run custom scenarios
        static std::wstring const ScenariosCommand;

        static std::wstring const HttpGateway;

        static std::wstring const EseDump;

        static std::wstring const XcopyCommand;

        static std::wstring const VectoredExceptionHandlerRegistration;

        // Security
        static std::wstring const NewTestCertificates;
        static std::wstring const UseAdminClient;
        static std::wstring const UseReadonlyClient;

        // Replicator wait for LSN
        static std::wstring const WaitForAllToApplyLsn;

        static std::wstring const RequestCheckpoint;

        // Some test scenarios were written to depend on behavior 
        // and features specific to KVS/ESE and will not work
        // when TStore is enabled for TestPersistedStoreService
        // (Linux only supports KVS/TStore)
        //
        static std::wstring const SetEseOnly;
        static std::wstring const ClearEseOnly;

        // Verify configured checkpoint and truncation interval timestamps
        // Intended for use with random tests only
        // Valid for TXRServiceTypes exclusively
        static std::wstring const EnableLogTruncationTimestampValidation;
    };
};
