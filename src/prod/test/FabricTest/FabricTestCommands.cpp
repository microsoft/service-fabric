// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace FabricTest;

wstring const FabricTestCommands::GetPlacementDataCommand(L"placementdata");
wstring const FabricTestCommands::KillCodePackageCommand(L"killcodep");
wstring const FabricTestCommands::PutDataCommand(L"putdata");
wstring const FabricTestCommands::GetDataCommand(L"getdata");
wstring const FabricTestCommands::FailFastCommand(L"fail");
wstring const FabricTestCommands::SetCodePackageKillPendingCommand(L"setcodepackagekillpending");

wstring const FabricTestCommands::AddCommand = L"+";
wstring const FabricTestCommands::RemoveCommand = L"-";
wstring const FabricTestCommands::AbortCommand = L"abort";
wstring const FabricTestCommands::RemoveDataParameter = L"removedata";
wstring const FabricTestCommands::ClearTicketCommand = L"clearticket";
wstring const FabricTestCommands::ResetStoreCommand = L"resetstore";
wstring const FabricTestCommands::CleanTestCommand = L"cleantest";
wstring const FabricTestCommands::SetPropertyCommand = L"set";
wstring const FabricTestCommands::DefineFailoverManagerServiceCommand = L"fmservice";
wstring const FabricTestCommands::DefineNamingServiceCommand = L"namingservice";
wstring const FabricTestCommands::DefineClusterManagerServiceCommand = L"cmservice";
wstring const FabricTestCommands::DefineImageStoreServiceCommand = L"imagestoreservice";
wstring const FabricTestCommands::DefineInfrastructureServiceCommand = L"infraservice";
wstring const FabricTestCommands::RemoveInfrastructureServiceCommand = L"removeinfraservice";
wstring const FabricTestCommands::DefineRepairManagerServiceCommand = L"rmservice";
wstring const FabricTestCommands::DefineDnsServiceCommand = L"dnsservice";
wstring const FabricTestCommands::DefineNIMServiceCommand = L"networkinventorymanager";
wstring const FabricTestCommands::DefineEnableUnsupportedPreviewFeaturesCommand = L"enableunsupportedpreviewfeatures";
wstring const FabricTestCommands::DefineMockImageBuilderCommand = L"mockimagebuilder";

wstring const FabricTestCommands::ListCommand = L"list";
wstring const FabricTestCommands::FMCommand = L"fm";
wstring const FabricTestCommands::UpdateServicePackageVersionInstanceCommand = L"updatespvi";
wstring const FabricTestCommands::ShowNodesCommand = L"nodes";
wstring const FabricTestCommands::ShowGFUMCommand = L"gfum";
wstring const FabricTestCommands::ShowLFUMCommand = L"lfum";
wstring const FabricTestCommands::ShowActiveServicesCommand = L"activeservices";
wstring const FabricTestCommands::ShowLoadMetricInfoCommand = L"loadmetrics";
wstring const FabricTestCommands::VerifyCommand = L"verify";
wstring const FabricTestCommands::VerifyFromFMCommand = L"verifyfromfm";

wstring const FabricTestCommands::CreateNameCommand = L"createname";
wstring const FabricTestCommands::DeleteNameCommand = L"deletename";
wstring const FabricTestCommands::DnsNameExistsCommand = L"dnsnameexists";
wstring const FabricTestCommands::NameExistsCommand = L"nameexists";
wstring const FabricTestCommands::PutPropertyCommand = L"putproperty";
wstring const FabricTestCommands::PutCustomPropertyCommand = L"putcustomproperty";
wstring const FabricTestCommands::SubmitPropertyBatchCommand = L"submitbatch";
wstring const FabricTestCommands::DeletePropertyCommand = L"deleteproperty";
wstring const FabricTestCommands::GetPropertyCommand = L"getproperty";
wstring const FabricTestCommands::GetMetadataCommand = L"getmetadata";
wstring const FabricTestCommands::EnumerateNamesCommand = L"enumnames";
wstring const FabricTestCommands::EnumeratePropertiesCommand = L"enumproperties";
wstring const FabricTestCommands::StartTestFabricClientCommand = L"startfabricclient";
wstring const FabricTestCommands::StopTestFabricClientCommand = L"stopfabricclient";
wstring const FabricTestCommands::VerifyPropertyEnumerationCommand = L"verifypropertyenumeration";

wstring const FabricTestCommands::CreateServiceGroupCommand = L"createservicegroup";
wstring const FabricTestCommands::UpdateServiceGroupCommand = L"updateservicegroup";
wstring const FabricTestCommands::DeleteServiceGroupCommand = L"deleteservicegroup";
wstring const FabricTestCommands::InjectFailureCommand = L"injectfailure";
wstring const FabricTestCommands::RemoveFailureCommand = L"removefailure";
wstring const FabricTestCommands::SetSignalCommand = L"setsignal";
wstring const FabricTestCommands::ResetSignalCommand = L"resetsignal";
wstring const FabricTestCommands::WaitForSignalHitCommand = L"waitforsignalhit";
wstring const FabricTestCommands::FailIfSignalHitCommand = L"failifsignalhit";
wstring const FabricTestCommands::WaitAsyncCommand = L"waitasync";
wstring const FabricTestCommands::QueryCommand = L"query";
wstring const FabricTestCommands::ExecuteQueryCommand = L"queryex";
wstring const FabricTestCommands::ReportHealthCommand = L"reporthealth";
wstring const FabricTestCommands::ReportHealthIpcCommand = L"reporthealthipc";
wstring const FabricTestCommands::ReportHealthInternalCommand = L"reporthealthinternal";
wstring const FabricTestCommands::DeleteHealthCommand = L"deletehealth";
wstring const FabricTestCommands::QueryHealthCommand = L"queryhealth";
wstring const FabricTestCommands::QueryHealthListCommand = L"queryhealthlist";
wstring const FabricTestCommands::QueryHealthStateChunkCommand = L"queryhealthstatechunk";
wstring const FabricTestCommands::CheckHMEntityCommand = L"checkhmentity";
wstring const FabricTestCommands::CorruptHMEntityCommand = L"corrupthmentity";
wstring const FabricTestCommands::CheckHMCommand = L"checkhm";
wstring const FabricTestCommands::SetHMThrottleCommand = L"sethmthrottle";
wstring const FabricTestCommands::TakeClusterHealthSnapshotCommand = L"takeclusterhealthsnapshot";
wstring const FabricTestCommands::CheckIsClusterHealthyCommand = L"checkisclusterhealthy";
wstring const FabricTestCommands::CheckHealthClientCommand = L"checkhealthclient";
wstring const FabricTestCommands::HealthPreInitializeCommand = L"healthpreinitialize";
wstring const FabricTestCommands::HealthPostInitializeCommand = L"healthpostinitialize";
wstring const FabricTestCommands::HealthGetProgressCommand = L"healthgetprogress";
wstring const FabricTestCommands::HealthSkipSequenceCommand = L"healthskipsequence";
wstring const FabricTestCommands::ResetHealthClientCommand = L"resethealthclient";
wstring const FabricTestCommands::WatchDogCommand = L"watchdog";
wstring const FabricTestCommands::HMLoadTestCommand = L"hmload";
wstring const FabricTestCommands::SetFabricClientSettingsCommand = L"setclientsettings";
wstring const FabricTestCommands::CreateServiceCommand = L"createservice";
wstring const FabricTestCommands::UpdateServiceCommand = L"updateservice";
wstring const FabricTestCommands::CreateServiceFromTemplateCommand = L"createservicefromtemplate";
wstring const FabricTestCommands::DeleteServiceCommand = L"deleteservice";
wstring const FabricTestCommands::ResolveServiceCommand = L"resolve";
wstring const FabricTestCommands::ResolveServiceWithEventsCommand = L"resolvewithevents";
wstring const FabricTestCommands::PrefixResolveServiceCommand = L"prefixresolve";
wstring const FabricTestCommands::CreateServiceNotificationClient = L"notificationclient";
wstring const FabricTestCommands::RegisterServiceNotificationFilter = L"notification+";
wstring const FabricTestCommands::UnregisterServiceNotificationFilter = L"notification-";
wstring const FabricTestCommands::SetServiceNotificationWait = L"setnotificationwait";
wstring const FabricTestCommands::ServiceNotificationWait = L"notificationwait";
wstring const FabricTestCommands::GetServiceDescriptionCommand = L"getservicedescription";
wstring const FabricTestCommands::GetServiceDescriptionUsingClientCommand = L"getservicedescriptionusingclient";
wstring const FabricTestCommands::GetServiceGroupDescriptionCommand = L"getservicegroupdescription";

wstring const FabricTestCommands::DeactivateNodeCommand = L"deactivatenode";
wstring const FabricTestCommands::ActivateNodeCommand = L"activatenode";

wstring const FabricTestCommands::DeactivateNodesCommand = L"deactivatenodes";
wstring const FabricTestCommands::RemoveNodeDeactivationCommand = L"removenodedeactivation";
wstring const FabricTestCommands::VerifyNodeDeactivationStatusCommand = L"verifynodedeactivationstatus";
wstring const FabricTestCommands::UpdateNodeImagesCommand = L"updatenodeimages";

wstring const FabricTestCommands::NodeStateRemovedCommand = L"nodestateremoved";
wstring const FabricTestCommands::RecoverPartitionsCommand = L"recoverpartitions";
wstring const FabricTestCommands::RecoverPartitionCommand = L"recoverpartition";
wstring const FabricTestCommands::RecoverServicePartitionsCommand = L"recoverservicepartitions";
wstring const FabricTestCommands::RecoverSystemPartitionsCommand = L"recoversystempartitions";
wstring const FabricTestCommands::ClientReportFaultCommand = L"clientreportfault";
wstring const FabricTestCommands::ResetPartitionLoadCommand = L"resetload";
wstring const FabricTestCommands::ToggleVerboseServicePlacementHealthReportingCommand = L"toggleverboseserviceplacementhealthreporting";

wstring const FabricTestCommands::ParallelProvisionApplicationTypeCommand = L"parallelprovisionapp";
wstring const FabricTestCommands::ProvisionApplicationTypeCommand = L"provisionapp";
wstring const FabricTestCommands::UnprovisionApplicationTypeCommand = L"unprovisionapp";
wstring const FabricTestCommands::CreateApplicationCommand = L"createapp";
wstring const FabricTestCommands::UpdateApplicationCommand = L"updateapp";
wstring const FabricTestCommands::DeleteApplicationCommand = L"deleteapp";
wstring const FabricTestCommands::UpgradeApplicationCommand = L"upgradeapp";
wstring const FabricTestCommands::UpdateApplicationUpgradeCommand = L"updateappupgrade";
wstring const FabricTestCommands::VerifyApplicationUpgradeDescriptionCommand = L"verifyappupgradedesc";
wstring const FabricTestCommands::RollbackApplicationUpgradeCommand = L"rollbackapp";
wstring const FabricTestCommands::SetRollbackApplicationCommand = L"setrollbackapp";
wstring const FabricTestCommands::UpgradeAppStatusCommand = L"upgradeappstatus";
wstring const FabricTestCommands::UpgradeAppMoveNext = L"upgradeappmovenext";
wstring const FabricTestCommands::VerifyUpgradeAppCommand = L"verifyupgradeapp";
wstring const FabricTestCommands::EnableNativeImageStore = L"enablenativeimagestore";
wstring const FabricTestCommands::VerifyImageStore = L"verifyimagestore";
wstring const FabricTestCommands::VerifyNodeFiles = L"verifynodefiles";
wstring const FabricTestCommands::DeployServicePackageCommand = L"deployservicepackagetonode";
wstring const FabricTestCommands::VerifyDeployedCodePackageCountCommand = L"verifydeployedcodepackagecount";

wstring const FabricTestCommands::CreateComposeCommand = L"createcompose";
wstring const FabricTestCommands::DeleteComposeCommand = L"deletecompose";
wstring const FabricTestCommands::UpgradeComposeCommand = L"upgradecompose";
wstring const FabricTestCommands::RollbackComposeCommand = L"rollbackcompose";

wstring const FabricTestCommands::CreateNetworkCommand = L"createnetwork";
wstring const FabricTestCommands::DeleteNetworkCommand = L"deletenetwork";
wstring const FabricTestCommands::GetNetworkCommand = L"getnetwork";
wstring const FabricTestCommands::ShowNetworksCommand = L"shownetworks";

wstring const FabricTestCommands::PrepareUpgradeFabricCommand = L"prepareupgradefabric";
wstring const FabricTestCommands::ProvisionFabricCommand = L"provisionfabric";
wstring const FabricTestCommands::UnprovisionFabricCommand = L"unprovisionfabric";
wstring const FabricTestCommands::UpgradeFabricCommand = L"upgradefabric";
wstring const FabricTestCommands::UpdateFabricUpgradeCommand = L"updatefabricupgrade";
wstring const FabricTestCommands::VerifyFabricUpgradeDescriptionCommand = L"verifyfabricupgradedesc";
wstring const FabricTestCommands::RollbackFabricUpgradeCommand = L"rollbackfabric";
wstring const FabricTestCommands::SetRollbackFabricCommand = L"setrollbackfabric";
wstring const FabricTestCommands::UpgradeFabricStatusCommand = L"upgradefabricstatus";
wstring const FabricTestCommands::UpgradeFabricMoveNext = L"upgradefabricmovenext";
wstring const FabricTestCommands::VerifyUpgradeFabricCommand = L"verifyupgradefabric";

wstring const FabricTestCommands::FailFabricUpgradeDownloadCommand = L"failfabricupgradedownload";
wstring const FabricTestCommands::FailFabricUpgradeValidationCommand = L"failfabricupgradevalidation";
wstring const FabricTestCommands::FailFabricUpgradeCommand = L"failfabricupgrade";

wstring const FabricTestCommands::ClientPutCommand = L"clientput";
wstring const FabricTestCommands::ClientGetCommand = L"clientget";
wstring const FabricTestCommands::ClientGetAllCommand = L"clientgetall";
wstring const FabricTestCommands::ClientGetKeysCommand = L"clientgetkeys";
wstring const FabricTestCommands::ClientDeleteCommand = L"clientdelete";
wstring const FabricTestCommands::ClientBackup = L"clientbackup";
wstring const FabricTestCommands::ClientRestore = L"clientrestore";
wstring const FabricTestCommands::ClientCompressionTest = L"clientcompression";

wstring const FabricTestCommands::ReportFaultCommand = L"reportfault";
wstring const FabricTestCommands::ReportLoadCommand = L"reportload";
wstring const FabricTestCommands::ReportMoveCostCommand = L"reportmovecost";

wstring const FabricTestCommands::VerifyLoadReportCommand = L"verifyloadreport";
wstring const FabricTestCommands::VerifyLoadValueCommand = L"verifyloadvalue";
wstring const FabricTestCommands::VerifyMoveCostValueCommand = L"verifymovecostvalue";
wstring const FabricTestCommands::VerifyNodeLoadCommand = L"verifynodeload";
wstring const FabricTestCommands::VerifyPartitionLoadCommand = L"verifypartitionload";
wstring const FabricTestCommands::VerifyClusterLoadCommand = L"verifyclusterload";
wstring const FabricTestCommands::VerifyUnplacedReasonCommand = L"verifyunplacedreason";
wstring const FabricTestCommands::VerifyApplicationLoadCommand = L"verifyapplicationload";
wstring const FabricTestCommands::VerifyResourceOnNodeCommand = L"verifyresourceonnode";
wstring const FabricTestCommands::VerifyLimitsEnforcedCommand = L"verifylimits";
wstring const FabricTestCommands::VerifyPLBAndLRMSync = L"verifyplbandlrmsync";
wstring const FabricTestCommands::VerifyContainerPodsCommand = L"verifypods";

wstring const FabricTestCommands::VerifyReadWriteStatusCommand = L"verifyreadwritestatus";
wstring const FabricTestCommands::CheckIfLFUPMIsEmptyCommand = L"checkiflfumpisempty";

wstring const FabricTestCommands::SetSecondaryPumpEnabledCommand = L"setsecondarypumpenabled";

wstring const FabricTestCommands::AddBehaviorCommand = L"client.addbehavior";
wstring const FabricTestCommands::RemoveBehaviorCommand = L"client.removebehavior";
wstring const FabricTestCommands::CheckUnreliableTransportIsDisabledCommand = L"checkunreliabletransportisdisabled";
wstring const FabricTestCommands::CheckTransportBehaviorlist = L"checktransportbehaviorlist";
wstring const FabricTestCommands::AddLeaseBehaviorCommand = L"addleasebehavior";
wstring const FabricTestCommands::RemoveLeaseBehaviorCommand = L"removeleasebehavior";

wstring const FabricTestCommands::StartClientCommand = L"startclient";
wstring const FabricTestCommands::StopClientCommand = L"stopclient";
wstring const FabricTestCommands::UseBackwardsCompatibleClientsCommand = L"usebackwardscompatibleclients";

wstring const FabricTestCommands::AddRuntimeCommand = L"addruntime";
wstring const FabricTestCommands::RemoveRuntimeCommand = L"removeruntime";
wstring const FabricTestCommands::UnregisterRuntimeCommand = L"unregisterruntime";
wstring const FabricTestCommands::RegisterServiceTypeCommand = L"addservicetype";
wstring const FabricTestCommands::EnableServiceTypeCommand = L"enableservicetype";
wstring const FabricTestCommands::DisableServiceTypeCommand = L"disableservicetype";

wstring const FabricTestCommands::InfrastructureCommand = L"infra";

wstring const FabricTestCommands::CreateRepairCommand = L"createrepair";
wstring const FabricTestCommands::CancelRepairCommand = L"cancelrepair";
wstring const FabricTestCommands::ForceApproveRepairCommand = L"forceapproverepair";
wstring const FabricTestCommands::DeleteRepairCommand = L"deleterepair";
wstring const FabricTestCommands::UpdateRepairCommand = L"updaterepair";
wstring const FabricTestCommands::ShowRepairsCommand = L"repairs";
wstring const FabricTestCommands::UpdateRepairHealthPolicyCommand = L"updaterepairhealthpolicy";
wstring const FabricTestCommands::GetRepairCommand = L"getrepair";

wstring const FabricTestCommands::ProcessPendingPlbUpdates = L"processpendingplbupdates";

wstring const FabricTestCommands::SwapPrimaryCommand = L"swapprimary";
wstring const FabricTestCommands::PromoteToPrimaryCommand = L"promotetoprimary";

wstring const FabricTestCommands::MovePrimaryFromClientCommand = L"moveprimaryclient";

wstring const FabricTestCommands::MoveSecondaryCommand = L"movesecondary";

wstring const FabricTestCommands::MoveSecondaryFromClientCommand = L"movesecondaryclient";

wstring const FabricTestCommands::PLBUpdateService = L"plbupdateservice";

wstring const FabricTestCommands::CreateClientCommand = L"createclient";
wstring const FabricTestCommands::DeleteClientCommand = L"deleteclient";
wstring const FabricTestCommands::RegisterCallbackCommand = L"registercallback";
wstring const FabricTestCommands::UnregisterCallbackCommand = L"unregistercallback";
wstring const FabricTestCommands::WaitForCallbackCommand = L"waitforcallback";
wstring const FabricTestCommands::VerifyCachedServiceResolutionCommand = L"verifycachedserviceresolution";
wstring const FabricTestCommands::RemoveCachedServiceResolutionCommand = L"removecachedserviceresolution";
wstring const FabricTestCommands::ResolveUsingClientCommand = L"resolveusingclient";
wstring const FabricTestCommands::VerifyFabricTimeCommand = L"verifyfabrictime";

wstring const FabricTestCommands::Repro789586Command = L"repro789586";
wstring const FabricTestCommands::PerformanceTestCommand = L"perftest";

wstring const FabricTestCommands::KillServiceCommand = L"killservice";
wstring const FabricTestCommands::KillFailoverManagerServiceCommand = L"killfmservice";
wstring const FabricTestCommands::KillNamingServiceCommand = L"killnamingservice";

wstring const FabricTestCommands::AddServiceBehaviorCommand = L"addservicebehavior";
wstring const FabricTestCommands::RemoveServiceBehaviorCommand = L"removeservicebehavior";

wstring const FabricTestCommands::DebugBreakCommand = L"dbgbreak";
wstring const FabricTestCommands::InjectErrorCommand = L"injecterror";
wstring const FabricTestCommands::RemoveErrorCommand = L"removeerror";

wstring const FabricTestCommands::EnableTransportThrottleCommand = L"enabletransportthrottle";
wstring const FabricTestCommands::ReloadTransportThrottleCommand = L"reloadtransportthrottle";

wstring const FabricTestCommands::CallService = L"call";

wstring const FabricTestCommands::MakeDropCommand = L"makedrop";

wstring const FabricTestCommands::ScenariosCommand = L"scenario";

wstring const FabricTestCommands::HttpGateway = L"httpgateway";

wstring const FabricTestCommands::EseDump = L"esedump";

wstring const FabricTestCommands::AddDiskDriveFoldersCommand = L"adddiskdrivefolders";
wstring const FabricTestCommands::VerifyDeletedDiskDriveFoldersCommand = L"verifydeleteddiskdrivefolders";

wstring const FabricTestCommands::RequestCheckpoint = L"requestcheckpoint";

wstring const FabricTestCommands::CheckContainersAvailable = L"checkcontainers";

/// <summary>
/// The xcopy command for copying directories.
/// </summary>
wstring const FabricTestCommands::XcopyCommand = L"xcopy";

//
// This registers a Vectored Exception handler, that crashes the process when there is a stack overflow. This 
// triggers the crash before the stack unwinding is done. Having this setting will affect perf, so it should 
// be enabled only if we want to debug a stack overflow. 
//
wstring const FabricTestCommands::VectoredExceptionHandlerRegistration = L"registerstackoverflowdumpgenerator";

wstring const FabricTestCommands::NewTestCertificates = L"newcerts";
wstring const FabricTestCommands::UseAdminClient = L"useadminclient";
wstring const FabricTestCommands::UseReadonlyClient = L"usereadonlyclient";

wstring const FabricTestCommands::WaitForAllToApplyLsn = L"waitforalltoapplylsn";

wstring const FabricTestCommands::SetEseOnly = L"seteseonly";
wstring const FabricTestCommands::ClearEseOnly = L"cleareseonly";

wstring const FabricTestCommands::EnableLogTruncationTimestampValidation = L"enablelogtruncationtimestampvalidation";