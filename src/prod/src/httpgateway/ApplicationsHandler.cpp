// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Query;
using namespace ServiceModel;
using namespace Naming;
using namespace Management::ClusterManager;
using namespace Management::FaultAnalysisService;
using namespace HttpGateway;

StringLiteral const TraceType("ApplicationsHandler");

ApplicationsHandler::ApplicationsHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ApplicationsHandler::~ApplicationsHandler()
{
}

ErrorCode ApplicationsHandler::Initialize()
{
    vector<HandlerUriTemplate> handlerUris;

    GetApplicationApiHandlers(handlerUris);
    validHandlerUris.insert(validHandlerUris.begin(), handlerUris.begin(), handlerUris.end());
    GetServiceApiHandlers(handlerUris);
    validHandlerUris.insert(validHandlerUris.begin(), handlerUris.begin(), handlerUris.end());
    GetPartitionApiHandlers(handlerUris);
    validHandlerUris.insert(validHandlerUris.begin(), handlerUris.begin(), handlerUris.end());

    auto error = server_.InnerServer->RegisterHandler(Constants::ApplicationsHandlerPath, shared_from_this());
    if (!error.IsSuccess()) { return error; }

    error = server_.InnerServer->RegisterHandler(Constants::ServicesHandlerPath, shared_from_this());
    if (!error.IsSuccess()) { return error; }

    return server_.InnerServer->RegisterHandler(Constants::PartitionsHandlerPath, shared_from_this());
}

//
// Populates all the handlers for Uri's under the root /Applications
//
void ApplicationsHandler::GetApplicationApiHandlers(vector<HandlerUriTemplate> &handlerUris)
{
    vector<HandlerUriTemplate> uris;

    // V2 supports continuation token
    // V6 supports hierarchical name delimiter
    uris.push_back(HandlerUriTemplate(
        Constants::ApplicationsEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllApplications)));

    uris.push_back(HandlerUriTemplate(
        Constants::ApplicationsEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetApplicationById)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::Delete),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteApplication)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntitySetPath, Constants::Create),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateApplication)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntitySetPathViaApplication, Constants::CreateServiceFromTemplate),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateServiceFromTemplate)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceGroupsEntitySetPathViaApplication, Constants::CreateServiceFromTemplate),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateServiceGroupFromTemplate)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceGroupsEntitySetPathViaApplication, Constants::CreateServiceFromTemplate),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateServiceGroupFromTemplate)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntitySetPathViaApplication, Constants::Create),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateService)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntitySetPathViaApplication, Constants::CreateServiceGroup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateServiceGroup)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceGroupsEntitySetPathViaApplication, Constants::Create),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateServiceGroup)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::Upgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpgradeApplication)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::UpdateUpgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateApplicationUpgrade)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::RollbackUpgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RollbackApplicationUpgrade)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::GetUpgradeProgress),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetUpgradeProgress)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::MoveToNextUpgradeDomain),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(MoveNextUpgradeDomain)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateApplicationHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateApplicationHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportApplicationHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetApplicationLoadInformation)));

#if !defined(PLATFORM_UNIX)
    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::EnableBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::GetBackupConfigurationInfo),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::DisableBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::SuspendBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::ResumeBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsEntityKeyPath, Constants::GetBackups),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));
#endif

    // V2 supports continuation token
    uris.push_back(HandlerUriTemplate(
        Constants::ServicesEntitySetPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllServices)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServicesEntityKeyPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceById)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::Update),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateService)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::UpdateServiceGroup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateServiceGroup)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceGroupsEntityKeyPathViaApplication, Constants::Update),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateServiceGroup)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::Delete),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteService)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::DeleteServiceGroup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteServiceGroup)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceGroupsEntityKeyPathViaApplication, Constants::Delete),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteServiceGroup)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::GetServiceDescription),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceDescription)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::GetServiceGroupDescription),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceGroupDescription)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::GetServiceGroupMembers),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceGroupMembers)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateServiceHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateServiceHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServiceHealth)));

    // /Applications/{0}/$/GetServices/{1}/$/ResolvePartition?api-version=1.0
    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::ResolvePartition),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ResolveServicePartition)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::ResolvePartition),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ResolveServicePartition)));

    // V2 supports continuation token
    uris.push_back(HandlerUriTemplate(
        Constants::ServicePartitionEntitySetPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllPartitions)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntitySetPathViaApplication, Constants::Recover),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverAllPartitions)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServicePartitionEntityKeyPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionByIdAndServiceName)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServicePartitionEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionByPartitionId)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaApplication, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluatePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaApplication, Constants::Recover),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverPartition)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaApplication, Constants::ResetLoad),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ResetPartitionLoad)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaApplication, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluatePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaApplication, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaApplication, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionLoadInformation)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicaEntitySetPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllReplicas)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicaEntityKeyPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetReplicaById)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaApplication, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaApplication, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaApplication, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePartitionReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPathViaApplication, Constants::GetUnplacedReplicaInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetUnplacedReplicaInformation)));

    uris.push_back(HandlerUriTemplate(
        Constants::NetworksEntitySetPathViaApplication,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllApplicationNetworks)));

    handlerUris = move(uris);
}

//
// Populates all the handlers for Uri's under the root /Services /ServiceGroups
//
void ApplicationsHandler::GetServiceApiHandlers(vector<HandlerUriTemplate> &handlerUris)
{
    vector<HandlerUriTemplate> uris;
    HandlerUriTemplate handlerUri;

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::Update),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateService)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::Delete),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteService)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetServiceDescription),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceDescription)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetServiceGroupDescription),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceGroupDescription)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateServiceHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateServiceHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServiceHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetApplicationName),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetApplicationName)));

    //	ServicesEntitySetPath
        // /Services/{0}/$/ResolvePartition?api-version=1.0
    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::ResolvePartition),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ResolveServicePartition)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::ResolvePartition),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ResolveServicePartition)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::StartPartitionDataLoss),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartPartitionDataLoss)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::StartPartitionQuorumLoss),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartPartitionQuorumLoss)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::StartPartitionRestart),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartPartitionRestart)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServicePartitionEntitySetPathViaService,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllPartitions)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntitySetPathViaService, Constants::Recover),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverAllPartitions)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServicePartitionEntityKeyPathViaService,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionByIdAndServiceName)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaService, Constants::Recover),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverPartition)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaService, Constants::ResetLoad),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ResetPartitionLoad)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaService, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluatePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaService, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluatePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaService, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaService, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionLoadInformation)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicaEntitySetPathViaService,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllReplicas)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicaEntityKeyPathViaService,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetReplicaById)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaService, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaService, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaService, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePartitionReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetUnplacedReplicaInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetUnplacedReplicaInformation)));

#if !defined(PLATFORM_UNIX)
    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::EnableBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetBackupConfigurationInfo),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::DisableBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::SuspendBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::ResumeBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicesEntityKeyPath, Constants::GetBackups),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));
#endif

    handlerUris = move(uris);
}

//
// Populates all the handlers for Uri's under the root /Partitions
//
void ApplicationsHandler::GetPartitionApiHandlers(vector<HandlerUriTemplate> &handlerUris)
{
    vector<HandlerUriTemplate> uris;

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluatePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluatePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::Recover),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverPartition)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::ResetLoad),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ResetPartitionLoad)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePartitionHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetServiceName),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceName)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicaEntitySetPathViaPartition,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllReplicas)));

    uris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicaEntityKeyPathViaPartition,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetReplicaById)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartition, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartition, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartition, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePartitionReplicaHealth)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartition, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetReplicaLoadInformation)));

    /* /Partitions/{0}/$/GetLoadInformation?api-version=1.0 */
    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionLoadInformation)));

#if !defined(PLATFORM_UNIX)
    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::EnableBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetBackupConfigurationInfo),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::DisableBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::SuspendBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::ResumeBackup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetBackups),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::Restore),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetRestoreProgress),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::Backup),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPath, Constants::GetBackupProgress),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(PerformBackupRestoreOperation),
        false));
#endif

    handlerUris = move(uris);
}

void ApplicationsHandler::GetAllApplications(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    ApplicationQueryDescription queryDescription;

    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion)
    {
        auto error = GetApplicationQueryDescription(thisSPtr, false, queryDescription);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllApplicationsComplete(operation, false);
    },
        thisSPtr);

    OnGetAllApplicationsComplete(operation, true);
}

void ApplicationsHandler::OnGetAllApplicationsComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ApplicationQueryResult> applicationsResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetApplicationList(operation, applicationsResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    bool useDelimiter = false;
    vector<ApplicationQueryResultWrapper> applicationResultWrapper;
    for (ULONG i = 0; i < applicationsResult.size(); i++)
    {
        if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
        {
            useDelimiter = true;
        }

        ApplicationQueryResultWrapper wrapper(move(applicationsResult[i]), useDelimiter);
        applicationResultWrapper.push_back(move(wrapper));
    }

    ByteBufferUPtr bufferUPtr;
    if (handlerOperation->Uri.ApiVersion == Constants::V1ApiVersion)
    {
        error = handlerOperation->Serialize(applicationResultWrapper, bufferUPtr);
    }
    else
    {
        ApplicationList list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }

        list.Items = move(applicationResultWrapper);

        error = handlerOperation->Serialize(list, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetApplicationById(AsyncOperationSPtr const& thisSPtr)
{
    NamingUri nameUri;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ApplicationQueryDescription queryDescription;

    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion)
    {
        auto error = GetApplicationQueryDescription(thisSPtr, true, queryDescription);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }
    else
    {
        // If API version is 1.0
        auto error = argumentParser.TryGetApplicationName(nameUri);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.ApplicationNameFilter = move(nameUri);
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetApplicationByIdComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetApplicationByIdComplete(operation, true);
}

ErrorCode ApplicationsHandler::GetApplicationQueryDescription(
    Common::AsyncOperationSPtr const & thisSPtr,
    bool includeApplicationName,
    __out ApplicationQueryDescription & queryDescription)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    UriArgumentParser argumentParser(handlerOperation->Uri);

    bool excludeApplicationParameters = false;

    ErrorCode error;

    // If an application name is given, the REST should return only on result - so there is no need for paging.
    if (includeApplicationName) // Get one item by ID
    {
        NamingUri nameUri;
        error = argumentParser.TryGetApplicationName(nameUri);
        if (!error.IsSuccess())
        {
            return error;
        }
        queryDescription.ApplicationNameFilter = move(nameUri);
    }
    else // Get list
    {
        wstring applicationTypeNameFilter;
        DWORD applicationDefinitionKindFilter = FABRIC_APPLICATION_DEFINITION_KIND_FILTER_DEFAULT;

        // Starting with Constants::V2ApiVersion, input can specify continuation token
        // Paging Status
        ServiceModel::QueryPagingDescription pagingDescription;
        error = argumentParser.TryGetPagingDescription(pagingDescription);
        if (!error.IsSuccess())
        {
            return error;
        }
        queryDescription.PagingDescription = make_unique<QueryPagingDescription>(move(pagingDescription));

        // ApplicationTypeName
        error = argumentParser.TryGetApplicationTypeName(applicationTypeNameFilter);
        if (error.IsSuccess())
        {
            queryDescription.ApplicationTypeNameFilter = move(applicationTypeNameFilter);
        }
        else if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::ApplicationTypeNotFound))
        {
            // If an error occurred and a applicationTypeNameFilter has been provided
            return error;
        }

        // ApplicationDefinitionKindFilter
        error = argumentParser.TryGetApplicationDefinitionKindFilter(applicationDefinitionKindFilter);
        if (error.IsSuccess())
        {
            queryDescription.ApplicationDefinitionKindFilter = applicationDefinitionKindFilter;
        }
        else if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // ExcludeApplicationParameters
    error = argumentParser.GetExcludeApplicationParameters(excludeApplicationParameters);

    if (error.IsSuccess())
    {
        queryDescription.ExcludeApplicationParameters = excludeApplicationParameters;
    }
    else if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        return error;
    }

    return ErrorCodeValue::Success;
}

void ApplicationsHandler::OnGetApplicationByIdComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ApplicationQueryResult> applicationsResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetApplicationList(operation, applicationsResult, pagingStatus);

    // We shouldn't receive paging status for just one application
    TESTASSERT_IF(pagingStatus, "OnGetApplicationByIdComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (applicationsResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    //
    // GetApplicationList with a appId should not return data for more than
    // 1 application.
    //
    if (applicationsResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    bool useDelimiter = false;
    if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
    {
        useDelimiter = true;
    }

    ApplicationQueryResultWrapper wrapper(move(applicationsResult[0]), useDelimiter);
    error = handlerOperation->Serialize(wrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::CreateApplication(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ApplicationDescriptionWrapper appDescData;
    auto error = handlerOperation->Deserialize(appDescData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginCreateApplication(
        appDescData,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateApplicationComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateApplicationComplete(inner, true);
}

void ApplicationsHandler::OnCreateApplicationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndCreateApplication(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusCreated, *Constants::StatusDescriptionCreated);
}

void ApplicationsHandler::DeleteApplication(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool isForce = false;
    error = argumentParser.TryGetForceRemove(isForce);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginDeleteApplication(
        DeleteApplicationDescription(
            move(appNameUri),
            isForce),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnDeleteApplicationComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnDeleteApplicationComplete(inner, true);
}

void ApplicationsHandler::OnDeleteApplicationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndDeleteApplication(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::UpgradeApplication(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ApplicationUpgradeDescriptionWrapper appUpgDescData;
    auto error = handlerOperation->Deserialize(appUpgDescData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginUpgradeApplication(
        appUpgDescData,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpgradeApplicationComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpgradeApplicationComplete(inner, true);
}

void ApplicationsHandler::OnUpgradeApplicationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndUpgradeApplication(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::UpdateApplicationUpgrade(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ApplicationUpgradeUpdateDescription updateDescription;
    auto error = handlerOperation->Deserialize(updateDescription, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginUpdateApplicationUpgrade(
        updateDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpdateApplicationUpgradeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpdateApplicationUpgradeComplete(inner, true);
}

void ApplicationsHandler::OnUpdateApplicationUpgradeComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndUpdateApplicationUpgrade(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::RollbackApplicationUpgrade(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginRollbackApplicationUpgrade(
        appNameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnRollbackApplicationUpgradeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRollbackApplicationUpgradeComplete(inner, true);
}

void ApplicationsHandler::OnRollbackApplicationUpgradeComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndRollbackApplicationUpgrade(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::GetUpgradeProgress(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginGetApplicationUpgradeProgress(
        appNameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetUpgradeProgressComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetUpgradeProgressComplete(inner, true);
}

void ApplicationsHandler::OnGetUpgradeProgressComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    IApplicationUpgradeProgressResultPtr resultPtr;

    auto error = client.AppMgmtClient->EndGetApplicationUpgradeProgress(operation, resultPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ApplicationUpgradeProgress upgradeProgress;
    error = upgradeProgress.FromInternalInterface(resultPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(upgradeProgress, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::MoveNextUpgradeDomain(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    MoveNextUpgradeDomainData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.AppMgmtClient->BeginMoveNextApplicationUpgradeDomain2(
        appNameUri,
        data.NextUpgradeDomain,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnMoveNextUpgradeDomainComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnMoveNextUpgradeDomainComplete(inner, true);
}

void ApplicationsHandler::OnMoveNextUpgradeDomainComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndMoveNextApplicationUpgradeDomain2(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::GetAllPartitions(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    // Starting with Constants::V2ApiVersion, input can specify continuation token
    wstring continuationToken;
    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion)
    {
        error = argumentParser.TryGetContinuationToken(continuationToken);
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    auto operation = client.QueryClient->BeginGetPartitionList(
        serviceNameUri,
        Guid::Empty(),
        continuationToken,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllPartitionsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetAllPartitionsComplete(operation, true);
}

void ApplicationsHandler::OnGetAllPartitionsComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServicePartitionQueryResult> partitionResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetPartitionList(operation, partitionResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (handlerOperation->Uri.ApiVersion == Constants::V1ApiVersion)
    {
        error = handlerOperation->Serialize(partitionResult, bufferUPtr);
    }
    else
    {
        PartitionList list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }

        if (handlerOperation->Uri.ApiVersion >= Constants::V64ApiVersion)
        {
            for (auto it = partitionResult.begin(); it != partitionResult.end(); ++it)
            {
                // Rename field CurrentConfigurationEpoch to PrimaryEpoch to match powershell output
                it->SetRenameAsPrimaryEpoch(true);
            }
        }

        list.Items = move(partitionResult);
        error = handlerOperation->Serialize(list, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
    return;
}

void ApplicationsHandler::GetPartitionByIdAndServiceName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    Guid partitionId;
    error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.QueryClient->BeginGetPartitionList(
        serviceNameUri,
        partitionId,
        EMPTY_STRING_QUERY_FILTER /*continuationToken*/,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetPartitionByIdAndServiceNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetPartitionByIdAndServiceNameComplete(operation, true);
}

void ApplicationsHandler::OnGetPartitionByIdAndServiceNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    vector<ServicePartitionQueryResult> partitionResult;

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetPartitionList(operation, partitionResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    // We shouldn't receive paging status for just one entry
    TESTASSERT_IF(pagingStatus, "OnGetPartitionByIdComplete: paging status shouldn't be set");

    ByteBufferUPtr bufferUPtr;
    if (partitionResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    if (partitionResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::OperationFailed));
        return;
    }

    error = handlerOperation->Serialize(partitionResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
    return;
}

void ApplicationsHandler::GetPartitionByPartitionId(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri emptyServiceName;
    auto operation = client.QueryClient->BeginGetPartitionList(
        emptyServiceName,
        partitionId,
        EMPTY_STRING_QUERY_FILTER /*continuationToken*/,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetPartitionByIdAndServiceNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetPartitionByIdAndServiceNameComplete(operation, true);
}

void ApplicationsHandler::GetAllReplicas(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    DWORD replicaStatus = FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT;
    error = argumentParser.TryGetReplicaStatusFilter(replicaStatus);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    // Starting with Constants::V2ApiVersion, input can specify continuation token
    wstring continuationToken;
    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion)
    {
        error = argumentParser.TryGetContinuationToken(continuationToken);
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    auto operation = client.QueryClient->BeginGetReplicaList(
        partitionId,
        0,
        replicaStatus,
        continuationToken,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllReplicasComplete(operation, false);
    },
        thisSPtr);

    OnGetAllReplicasComplete(operation, true);
}

void ApplicationsHandler::OnGetAllReplicasComplete(
    AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServiceReplicaQueryResult> serviceReplicasQueryResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetReplicaList(operation, serviceReplicasQueryResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (handlerOperation->Uri.ApiVersion == Constants::V1ApiVersion)
    {
        error = handlerOperation->Serialize(serviceReplicasQueryResult, bufferUPtr);
    }
    else
    {
        ReplicaList list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }

        list.Items = move(serviceReplicasQueryResult);

        error = handlerOperation->Serialize(list, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
    return;
}

void ApplicationsHandler::GetReplicaById(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    FABRIC_REPLICA_ID replicaId;
    error = argumentParser.TryGetRequiredReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    if (replicaId == 0)
    {
        // Don't allow 0 to be passed as replica id filter. The GetReplicaList
        // query uses the 0 replica id to mean don't filter on a replica id.
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Invalid_ReplicaId_Parameter), replicaId)));
        return;
    }

    DWORD replicaStatus = FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT;
    error = argumentParser.TryGetReplicaStatusFilter(replicaStatus);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.QueryClient->BeginGetReplicaList(
        partitionId,
        replicaId,
        replicaStatus,
        EMPTY_STRING_QUERY_FILTER /*continuationToken*/,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetReplicaByIdComplete(operation, false);
    },
        thisSPtr);

    OnGetReplicaByIdComplete(operation, true);
}

void ApplicationsHandler::OnGetReplicaByIdComplete(
    AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServiceReplicaQueryResult> serviceReplicasQueryResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetReplicaList(operation, serviceReplicasQueryResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    // We shouldn't receive paging status for just one entry
    TESTASSERT_IF(pagingStatus, "OnGetReplicaByIdComplete: paging status shouldn't be set");

    ByteBufferUPtr bufferUPtr;
    if (serviceReplicasQueryResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    if (serviceReplicasQueryResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::OperationFailed));
        return;
    }

    error = handlerOperation->Serialize(serviceReplicasQueryResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
    return;
}

void ApplicationsHandler::GetAllServices(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    NamingUri appNameUri;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    // Starting with Constants::V2ApiVersion, input can specify continuation token
    wstring serviceTypeNameFilter;
    ServiceModel::QueryPagingDescription pagingDescription;
    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion)
    {
        if (handlerOperation->Uri.ApiVersion >= Constants::V64ApiVersion)
        {
            error = argumentParser.TryGetPagingDescription(pagingDescription);
            if (!error.IsSuccess())
            {
                handlerOperation->OnError(thisSPtr, error);
                return;
            }
        }
        else
        {
            wstring continuationToken;
            error = argumentParser.TryGetContinuationToken(continuationToken);
            if (error.IsError(ErrorCodeValue::NameNotFound))
            {
                error = ErrorCode::Success();
            }
            pagingDescription.ContinuationToken = move(continuationToken);
        }

        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        error = argumentParser.TryGetServiceTypeName(serviceTypeNameFilter);
        if (error.IsError(ErrorCodeValue::ServiceTypeNotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    ServiceQueryDescription queryDescription(
        move(appNameUri),
        NamingUri(*EMPTY_URI_QUERY_FILTER),
        move(serviceTypeNameFilter),
        move(pagingDescription));

    AsyncOperationSPtr operation = client.QueryClient->BeginGetServiceList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllServicesComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetAllServicesComplete(operation, true);
}

void ApplicationsHandler::OnGetAllServicesComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServiceQueryResult> serviceResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetServiceList(operation, serviceResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    bool useDelimiter = false;
    vector<ServiceQueryResultWrapper> serviceResultWrapper;
    for (ULONG i = 0; i < serviceResult.size(); i++)
    {
        if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
        {
            useDelimiter = true;
        }

        ServiceQueryResultWrapper wrapper(serviceResult[i], useDelimiter);
        serviceResultWrapper.push_back(move(wrapper));
    }

    ByteBufferUPtr bufferUPtr;
    if (handlerOperation->Uri.ApiVersion == Constants::V1ApiVersion)
    {
        error = handlerOperation->Serialize(serviceResultWrapper, bufferUPtr);
    }
    else
    {
        ServiceList list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }

        list.Items = move(serviceResultWrapper);

        error = handlerOperation->Serialize(list, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetServiceById(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri appNameUri;
    NamingUri serviceNameFilter;

    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    error = argumentParser.TryGetServiceName(serviceNameFilter);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetServiceList(
        ServiceQueryDescription(
            move(appNameUri),
            move(serviceNameFilter),
            wstring(EMPTY_STRING_QUERY_FILTER) /*serviceTypeNameFilter*/,
            QueryPagingDescription()),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetServiceByIdComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetServiceByIdComplete(operation, true);
}

void ApplicationsHandler::OnGetServiceByIdComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServiceQueryResult> serviceResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetServiceList(operation, serviceResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    // We shouldn't receive paging status for just one service
    TESTASSERT_IF(pagingStatus, "OnGetServiceByIdComplete: paging status shouldn't be set");

    ByteBufferUPtr bufferUPtr;
    if (serviceResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    //
    // GetApplicationServiceList with a serviceId should not return data for more than
    // 1 service.
    //
    if (serviceResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::OperationFailed));
        return;
    }

    bool useDelimiter = false;
    if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
    {
        useDelimiter = true;
    }

    ServiceQueryResultWrapper wrapper(serviceResult[0], useDelimiter);
    error = handlerOperation->Serialize(wrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::SerializationError));
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::CreateServiceFromTemplate(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri applicationName;

    auto error = argumentParser.TryGetApplicationName(applicationName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    CreateServiceFromTemplateData cstData(applicationName.ToString());
    error = handlerOperation->Deserialize(cstData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (cstData.ApplicationName != applicationName.ToString())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    NamingUri serviceName;
    auto hr = NamingUri::TryParse(cstData.ServiceName.c_str(), Constants::HttpGatewayTraceId, serviceName);
    if (FAILED(hr))
    {
        handlerOperation->OnError(thisSPtr, ErrorCode::FromHResult(hr));
        return;
    }

    if (cstData.ServiceType.empty() || cstData.ServiceType.length() > ParameterValidator::MaxStringSize)
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceMgmtClient->BeginCreateServiceFromTemplate(
        ServiceFromTemplateDescription(
            applicationName,
            serviceName,
            cstData.ServiceDnsName,
            cstData.ServiceType,
            cstData.PackageActivationMode,
            cstData.InitData),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateServiceFromTemplateComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateServiceFromTemplateComplete(inner, true);
}

void ApplicationsHandler::OnCreateServiceFromTemplateComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceMgmtClient->EndCreateServiceFromTemplate(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::CreateServiceGroupFromTemplate(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri applicationName;

    auto error = argumentParser.TryGetApplicationName(applicationName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    CreateServiceGroupFromTemplateData cstData(applicationName.ToString());
    error = handlerOperation->Deserialize(cstData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (cstData.ApplicationName != applicationName.ToString())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    NamingUri serviceName;
    auto hr = NamingUri::TryParse(cstData.ServiceName.c_str(), Constants::HttpGatewayTraceId, serviceName);
    if (FAILED(hr))
    {
        handlerOperation->OnError(thisSPtr, ErrorCode::FromHResult(hr));
        return;
    }

    if (cstData.ServiceType.empty() || cstData.ServiceType.length() > ParameterValidator::MaxStringSize)
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceGroupMgmtClient->BeginCreateServiceGroupFromTemplate(
        ServiceGroupFromTemplateDescription(
            applicationName,
            serviceName,
            cstData.ServiceType,
            cstData.PackageActivationMode,
            cstData.InitData),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateServiceGroupFromTemplateComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateServiceGroupFromTemplateComplete(inner, true);
}

void ApplicationsHandler::OnCreateServiceGroupFromTemplateComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceGroupMgmtClient->EndCreateServiceGroupFromTemplate(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::CreateService(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    NamingUri applicationName;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    PartitionedServiceDescWrapper psd;

    auto error = argumentParser.TryGetApplicationName(applicationName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    psd.ApplicationName = applicationName.ToString();
    error = handlerOperation->Deserialize(psd, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (psd.ApplicationName != applicationName.ToString())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (psd.ServiceKind == FABRIC_SERVICE_KIND_STATEFUL)
    {
        for (auto m : psd.Metrics)
        {
            if ((m.Name == L"PrimaryCount" && (m.PrimaryDefaultLoad != 1 || m.SecondaryDefaultLoad != 0))
                || (m.Name == L"ReplicaCount" && (m.PrimaryDefaultLoad != 1 || m.SecondaryDefaultLoad != 1))
                || (m.Name == L"SecondaryCount" && (m.PrimaryDefaultLoad != 0 || m.SecondaryDefaultLoad != 1))
                || (m.Name == L"Count" && (m.PrimaryDefaultLoad != 1 || m.SecondaryDefaultLoad != 1))
                || (m.Name == L"InstanceCount"))
            {
                handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
                return;
            }
        }
    }
    else if (psd.ServiceKind == FABRIC_SERVICE_KIND_STATELESS)
    {
        for (auto m : psd.Metrics)
        {
            if ((m.Name == L"PrimaryCount")
                || (m.Name == L"ReplicaCount")
                || (m.Name == L"SecondaryCount")
                || (m.Name == L"Count" && m.PrimaryDefaultLoad != 1)
                || (m.Name == L"InstanceCount" && m.PrimaryDefaultLoad != 1))
            {
                handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
                return;
            }
        }
    }

    WriteNoise(
        TraceType,
        "CreateService(): AppName={0}, ServiceName={1}, ServiceDnsName={2} and PackageActivationMode={3}.",
        psd.ApplicationName,
        psd.ServiceName,
        psd.ServiceDnsName,
        psd.PackageActivationMode);

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceMgmtClient->BeginCreateService(
        psd,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateServiceComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateServiceComplete(inner, true);
}

void ApplicationsHandler::OnCreateServiceComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceMgmtClient->EndCreateService(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::CreateServiceGroup(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ServiceGroupDescriptionAdaptor serviceGroupDescriptionAdaptor;

    auto error = handlerOperation->Deserialize(serviceGroupDescriptionAdaptor, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    serviceGroupDescriptionAdaptor.ToPartitionedServiceDescWrapper();

    PartitionedServiceDescWrapper &psd = serviceGroupDescriptionAdaptor;

    auto inner = client.ServiceGroupMgmtClient->BeginCreateServiceGroup(
        psd,
        TimeSpan::FromMinutes(Constants::DefaultFabricTimeoutMin),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateServiceGroupComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateServiceGroupComplete(inner, true);
}

void ApplicationsHandler::OnCreateServiceGroupComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceGroupMgmtClient->EndCreateServiceGroup(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::UpdateService(AsyncOperationSPtr const& operation)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation);
    auto &client = handlerOperation->FabricClient;
    NamingUri serviceName;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation, error);
        return;
    }

    ServiceUpdateDescription updateDescription;
    error = handlerOperation->Deserialize(updateDescription, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceMgmtClient->BeginUpdateService(
        serviceName,
        updateDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpdateServiceComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpdateServiceComplete(inner, true);
}

void ApplicationsHandler::OnUpdateServiceComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceMgmtClient->EndUpdateService(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::UpdateServiceGroup(AsyncOperationSPtr const& operation)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation);
    auto &client = handlerOperation->FabricClient;
    NamingUri serviceName;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation, error);
        return;
    }

    ServiceUpdateDescription updateDescription;
    error = handlerOperation->Deserialize(updateDescription, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceGroupMgmtClient->BeginUpdateServiceGroup(
        serviceName,
        updateDescription,
        TimeSpan::FromMinutes(Constants::DefaultFabricTimeoutMin),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpdateServiceGroupComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpdateServiceGroupComplete(inner, true);
}

void ApplicationsHandler::OnUpdateServiceGroupComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceGroupMgmtClient->EndUpdateServiceGroup(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::DeleteService(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    auto &client = handlerOperation->FabricClient;

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool isForce = false;
    error = argumentParser.TryGetForceRemove(isForce);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceMgmtClient->BeginDeleteService(
        DeleteServiceDescription(
            move(serviceNameUri),
            isForce),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnDeleteServiceComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnDeleteServiceComplete(inner, true);
}

void ApplicationsHandler::OnDeleteServiceComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceMgmtClient->EndDeleteService(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::DeleteServiceGroup(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    auto &client = handlerOperation->FabricClient;

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceGroupMgmtClient->BeginDeleteServiceGroup(
        serviceNameUri,
        TimeSpan::FromMinutes(Constants::DefaultFabricTimeoutMin),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnDeleteServiceGroupComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnDeleteServiceGroupComplete(inner, true);
}

void ApplicationsHandler::OnDeleteServiceGroupComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ServiceGroupMgmtClient->EndDeleteServiceGroup(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::GetServiceDescription(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceMgmtClient->BeginGetServiceDescription(
        serviceNameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnServiceDescriptionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnServiceDescriptionComplete(inner, true);
}

void ApplicationsHandler::OnServiceDescriptionComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    PartitionedServiceDescriptor result;
    PartitionedServiceDescWrapper description;
    auto error = client.ServiceMgmtClient->EndGetServiceDescription(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    result.ToWrapper(description);
    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(description, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetServiceGroupDescription(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ServiceGroupMgmtClient->BeginGetServiceGroupDescription(
        serviceNameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnServiceGroupDescriptionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnServiceGroupDescriptionComplete(inner, true);
}

void ApplicationsHandler::OnServiceGroupDescriptionComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    ServiceGroupDescriptor result;

    auto error = client.ServiceGroupMgmtClient->EndGetServiceGroupDescription(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    PartitionedServiceDescriptor partitionedServiceDescriptor = result.get_PartitionedServiceDescriptor();
    CServiceGroupDescription cServiceGroupDescription = result.get_CServiceGroupDescription();
    ServiceGroupDescriptionAdaptor serviceGroupDescriptionAdaptor;
    partitionedServiceDescriptor.ToWrapper(serviceGroupDescriptionAdaptor);
    serviceGroupDescriptionAdaptor.FromPartitionedServiceDescWrapper(cServiceGroupDescription);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(serviceGroupDescriptionAdaptor, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetServiceGroupMembers(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NamingUri appNameUri;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri serviceNameUri;
    error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.QueryClient->BeginGetServiceGroupMemberList(
        appNameUri,
        serviceNameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnServiceGroupMembersComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnServiceGroupMembersComplete(inner, true);
}

void ApplicationsHandler::OnServiceGroupMembersComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    vector<ServiceGroupMemberQueryResult> serviceGroupMemberList;

    auto error = client.QueryClient->EndGetServiceGroupMemberList(operation, serviceGroupMemberList);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    if (serviceGroupMemberList.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::OperationFailed));
        return;
    }

    ServiceGroupMemberQueryResult serviceGroupMemberQueryResult;
    serviceGroupMemberQueryResult = move(serviceGroupMemberList[0]);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(serviceGroupMemberQueryResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::EvaluateApplicationHealth(AsyncOperationSPtr const& thisSPtr)
{
    unique_ptr<ApplicationHealthPolicy> healthPolicy;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri nameUri;
    auto error = argumentParser.TryGetApplicationName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ApplicationHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ApplicationHealthPolicy")));
            return;
        }
    }

    ApplicationHealthQueryDescription queryDescription(move(nameUri), move(healthPolicy));

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    wstring servicesFilterString;
    if (handlerOperation->Uri.GetItem(Constants::ServicesHealthStateFilterString, servicesFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(servicesFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetServicesFilter(make_unique<ServiceHealthStatesFilter>(filter));
    }

    wstring deployedApplicationsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::DeployedApplicationsHealthStateFilterString, deployedApplicationsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(deployedApplicationsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetDeployedApplicationsFilter(make_unique<DeployedApplicationHealthStatesFilter>(filter));
    }

    ApplicationHealthStatisticsFilterUPtr statsFilter;
    bool excludeStatsFilter = false;
    error = argumentParser.TryGetExcludeStatisticsFilter(excludeStatsFilter);
    if (error.IsSuccess())
    {
        statsFilter = make_unique<ApplicationHealthStatisticsFilter>(excludeStatsFilter);
        queryDescription.SetStatisticsFilter(move(statsFilter));
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.HealthClient->BeginGetApplicationHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->EvaluateApplicationHealthComplete(operation, false);
    },
        thisSPtr);

    EvaluateApplicationHealthComplete(operation, true);
}

void ApplicationsHandler::EvaluateApplicationHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ApplicationHealth health;
    auto error = client.HealthClient->EndGetApplicationHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::EvaluateServiceHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri nameUri;
    auto error = argumentParser.TryGetServiceName(nameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ApplicationHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ApplicationHealthPolicy")));
            return;
        }
    }

    ServiceHealthQueryDescription queryDescription(move(nameUri), move(healthPolicy));

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    wstring partitionsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::PartitionsHealthStateFilterString, partitionsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(partitionsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetPartitionsFilter(make_unique<PartitionHealthStatesFilter>(filter));
    }

    ServiceHealthStatisticsFilterUPtr statsFilter;
    bool excludeStatsFilter = false;
    error = argumentParser.TryGetExcludeStatisticsFilter(excludeStatsFilter);
    if (error.IsSuccess())
    {
        statsFilter = make_unique<ServiceHealthStatisticsFilter>(excludeStatsFilter);
        queryDescription.SetStatisticsFilter(move(statsFilter));
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetServiceHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->EvaluateServiceHealthComplete(operation, false);
    },
        thisSPtr);

    EvaluateServiceHealthComplete(operation, true);
}

void ApplicationsHandler::EvaluateServiceHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ServiceHealth health;
    auto error = client.HealthClient->EndGetServiceHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::EvaluatePartitionHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ApplicationHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ApplicationHealthPolicy")));
            return;
        }
    }

    PartitionHealthQueryDescription queryDescription(partitionId, move(healthPolicy));

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    wstring replicasFilterString;
    if (handlerOperation->Uri.GetItem(Constants::ReplicasHealthStateFilterString, replicasFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(replicasFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetReplicasFilter(make_unique<ReplicaHealthStatesFilter>(filter));
    }

    PartitionHealthStatisticsFilterUPtr statsFilter;
    bool excludeStatsFilter = false;
    error = argumentParser.TryGetExcludeStatisticsFilter(excludeStatsFilter);
    if (error.IsSuccess())
    {
        statsFilter = make_unique<PartitionHealthStatisticsFilter>(excludeStatsFilter);
        queryDescription.SetStatisticsFilter(move(statsFilter));
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.HealthClient->BeginGetPartitionHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->EvaluatePartitionHealthComplete(operation, false);
    },
        thisSPtr);

    EvaluatePartitionHealthComplete(operation, true);
}

void ApplicationsHandler::EvaluatePartitionHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    PartitionHealth health;
    auto error = client.HealthClient->EndGetPartitionHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::EvaluateReplicaHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    FABRIC_REPLICA_ID replicaId;
    error = argumentParser.TryGetRequiredReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ApplicationHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ApplicationHealthPolicy")));
            return;
        }
    }

    ReplicaHealthQueryDescription queryDescription(partitionId, replicaId, move(healthPolicy));

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetReplicaHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->EvaluateReplicaHealthComplete(operation, false);
    },
        thisSPtr);

    EvaluateReplicaHealthComplete(operation, true);
}

void ApplicationsHandler::EvaluateReplicaHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ReplicaHealth health;
    auto error = client.HealthClient->EndGetReplicaHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::RecoverAllPartitions(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri serviceName;

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.ClusterMgmtClient->BeginRecoverServicePartitions(
        serviceName.ToString(),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnRecoverAllPartitionsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRecoverAllPartitionsComplete(operation, true);
}

void ApplicationsHandler::OnRecoverAllPartitionsComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndRecoverServicePartitions(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::RecoverPartition(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.ClusterMgmtClient->BeginRecoverPartition(
        partitionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnRecoverPartitionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRecoverPartitionComplete(operation, true);
}

void ApplicationsHandler::OnRecoverPartitionComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndRecoverPartition(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::ResetPartitionLoad(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.ClusterMgmtClient->BeginResetPartitionLoad(
        partitionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnResetPartitionLoadComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnResetPartitionLoadComplete(operation, true);
}

void ApplicationsHandler::OnResetPartitionLoadComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndResetPartitionLoad(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ApplicationsHandler::ResolveServicePartition(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceName;
    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    PartitionKey partitionKey;
    error = argumentParser.TryGetPartitionKey(partitionKey);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // Get Previous RSP version if passed in
    //
    ResolvedServicePartitionMetadataSPtr rspMetadataSPtr;
    wstring previousRspVersion;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        PreviousRspVersionData previousVersionData;
        error = handlerOperation->Deserialize(previousVersionData, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        error = NamingUri::UnescapeString(previousVersionData.Version, previousRspVersion);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
        // TODO remove after JSON serialization of sptr is checked in
        rspMetadataSPtr = make_shared<ResolvedServicePartitionMetadata>();
        error = handlerOperation->Deserialize(*rspMetadataSPtr, previousRspVersion);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }
    else if (handlerOperation->Uri.GetItem(Constants::PreviousRspVersion, previousRspVersion) &&
        !previousRspVersion.empty())
    {
        wstring unescapedString;
        error = NamingUri::UnescapeString(previousRspVersion, unescapedString);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        // TODO remove after JSON serialization of sptr is checked in
        rspMetadataSPtr = make_shared<ResolvedServicePartitionMetadata>();
        error = handlerOperation->Deserialize(*rspMetadataSPtr, unescapedString);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    auto operation = client.ServiceMgmtClient->BeginResolveServicePartition(
        serviceName,
        partitionKey,
        rspMetadataSPtr,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnResolveServicePartitionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnResolveServicePartitionComplete(operation, true);
}

void ApplicationsHandler::OnResolveServicePartitionComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ResolvedServicePartitionSPtr rspSPtr;
    auto error = client.ServiceMgmtClient->EndResolveServicePartition(operation, rspSPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ResolvedServicePartitionWrapper rspWrapper;
    rspSPtr->ToWrapper(rspWrapper);

    //
    // User doesnt interpret the version string and it can be directly returned in query params or in body,
    // so it must be escaped correctly.
    //
    error = NamingUri::EscapeString(rspWrapper.Version, rspWrapper.Version);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(rspWrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::ReportApplicationHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri applicationName;
    auto error = argumentParser.TryGetApplicationName(applicationName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    //
    // Reporting health per Instance id is not exposed externally
    //
    AttributeList attributeList;
    HealthReport healthReport(
        EntityHealthInformation::CreateApplicationEntityHealthInformation(applicationName.ToString(), FABRIC_INVALID_INSTANCE_ID),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ApplicationsHandler::ReportServiceHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceName;
    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    AttributeList attributeList;
    HealthReport healthReport(
        EntityHealthInformation::CreateServiceEntityHealthInformation(serviceName.ToString(), FABRIC_INVALID_INSTANCE_ID),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ApplicationsHandler::ReportServicePartitionHealth(AsyncOperationSPtr const& thisSPtr)
{
    Guid partitionId;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    AttributeList attributeList;
    HealthReport healthReport(
        EntityHealthInformation::CreatePartitionEntityHealthInformation(partitionId),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ApplicationsHandler::ReportServicePartitionReplicaHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    FABRIC_REPLICA_ID replicaId;
    error = argumentParser.TryGetRequiredReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    FABRIC_SERVICE_KIND serviceKind;
    bool isServiceKindRequired = true;
    // Starting with apiVersion v4, ServiceKind is required.
    if (handlerOperation->Uri.ApiVersion <= Constants::V3ApiVersion)
    {
        isServiceKindRequired = false;
    }

    error = argumentParser.TryGetServiceKind(isServiceKindRequired, serviceKind);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    EntityHealthInformationSPtr entityInfo;
    if (serviceKind == FABRIC_SERVICE_KIND_STATEFUL)
    {
        entityInfo = EntityHealthInformation::CreateStatefulReplicaEntityHealthInformation(partitionId, replicaId, FABRIC_INVALID_INSTANCE_ID);
    }
    else
    {
        entityInfo = EntityHealthInformation::CreateStatelessInstanceEntityHealthInformation(partitionId, replicaId);
    }

    AttributeList attributeList;
    HealthReport healthReport(
        move(entityInfo),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ApplicationsHandler::GetPartitionLoadInformation(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.QueryClient->BeginGetPartitionLoadInformation(
        partitionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->GetPartitionLoadInformationComplete(operation, false);
    },
        thisSPtr);

    GetPartitionLoadInformationComplete(operation, true);
}

void ApplicationsHandler::GetPartitionLoadInformationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    PartitionLoadInformationQueryResult queryResult;
    auto error = client.QueryClient->EndGetPartitionLoadInformation(operation, queryResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(queryResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetReplicaLoadInformation(AsyncOperationSPtr const& thisSPtr)
{
    wstring partitionIdString;

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    FABRIC_REPLICA_ID replicaId;
    error = argumentParser.TryGetRequiredReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.QueryClient->BeginGetReplicaLoadInformation(
        partitionId,
        replicaId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->GetReplicaLoadInformationComplete(operation, false);
    },
        thisSPtr);

    GetReplicaLoadInformationComplete(operation, true);
}

void ApplicationsHandler::GetReplicaLoadInformationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ReplicaLoadInformationQueryResult queryResult;
    auto error = client.QueryClient->EndGetReplicaLoadInformation(operation, queryResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(queryResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetUnplacedReplicaInformation(AsyncOperationSPtr const& thisSPtr)
{
    NamingUri serviceName;
    bool onlyQueryPrimary;

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    Guid partitionId;
    error = argumentParser.TryGetOptionalPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        partitionId = Guid::Empty();
    }

    error = handlerOperation->Uri.GetBool(L"OnlyQueryPrimaries", onlyQueryPrimary);
    if (!error.IsSuccess())
    {
        onlyQueryPrimary = false;
    }

    auto operation = client.QueryClient->BeginGetUnplacedReplicaInformation(
        serviceName.Name,
        partitionId,
        onlyQueryPrimary,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->GetUnplacedReplicaInformationComplete(operation, false);
    },
        thisSPtr);

    GetUnplacedReplicaInformationComplete(operation, true);
}

void ApplicationsHandler::GetUnplacedReplicaInformationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    UnplacedReplicaInformationQueryResult queryResult;
    auto error = client.QueryClient->EndGetUnplacedReplicaInformation(operation, queryResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(queryResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::StartPartitionDataLoss(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri serviceName;

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    StartPartitionDataLossData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    Guid operationId = Guid::Empty();
    if (!Guid::TryParse(data.OperationId, Constants::HttpGatewayTraceId, operationId))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    FABRIC_PARTITION_SELECTOR partitionSelector;

    wstring tempString = serviceName.Name;
    partitionSelector.ServiceName = tempString.c_str();
    partitionSelector.PartitionSelectorType = data.PartitionSelectorType;
    partitionSelector.PartitionKey = data.PartitionKey.c_str();

    FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION dld;
    dld.OperationId = operationId.AsGUID();
    dld.PartitionSelector = &partitionSelector;
    dld.DataLossMode = data.DataLossMode;

    InvokeDataLossDescription description;
    error = description.FromPublicApi(dld);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "InvokeDataLossDescription.FromPublicApi failed with {0}", error);
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.TestMgmtClient->BeginInvokeDataLoss(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnStartPartitionDataLossComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnStartPartitionDataLossComplete(inner, true);
}

void ApplicationsHandler::OnStartPartitionDataLossComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndInvokeDataLoss(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::StartPartitionQuorumLoss(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri serviceName;

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    StartPartitionQuorumLossData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    Guid operationId = Guid::Empty();
    if (!Guid::TryParse(data.OperationId, Constants::HttpGatewayTraceId, operationId))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    FABRIC_PARTITION_SELECTOR partitionSelector;
    wstring tempString = serviceName.Name;
    partitionSelector.ServiceName = tempString.c_str();
    partitionSelector.PartitionSelectorType = data.PartitionSelectorType;
    partitionSelector.PartitionKey = data.PartitionKey.c_str();

    FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION qld;
    qld.OperationId = operationId.AsGUID();
    qld.PartitionSelector = &partitionSelector;
    qld.QuorumLossMode = data.QuorumLossMode;
    qld.QuorumLossDurationInMilliSeconds = data.QuorumLossDurationInSeconds;

    InvokeQuorumLossDescription description;
    error = description.FromPublicApi(qld);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "InvokeQuorumLossDescription.FromPublicApi failed with {0}", error);
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.TestMgmtClient->BeginInvokeQuorumLoss(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnStartPartitionQuorumLossComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnStartPartitionQuorumLossComplete(inner, true);
}

void ApplicationsHandler::OnStartPartitionQuorumLossComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndInvokeQuorumLoss(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::StartPartitionRestart(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    NamingUri serviceName;

    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    StartPartitionRestartData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    Guid operationId = Guid::Empty();
    if (!Guid::TryParse(data.OperationId, Constants::HttpGatewayTraceId, operationId))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    FABRIC_PARTITION_SELECTOR partitionSelector;
    wstring tempString = serviceName.Name;
    partitionSelector.ServiceName = tempString.c_str();
    partitionSelector.PartitionSelectorType = data.PartitionSelectorType;
    partitionSelector.PartitionKey = data.PartitionKey.c_str();

    FABRIC_START_PARTITION_RESTART_DESCRIPTION rd;
    rd.OperationId = operationId.AsGUID();
    rd.PartitionSelector = &partitionSelector;
    rd.RestartPartitionMode = data.RestartPartitionMode;

    RestartPartitionDescription description;
    error = description.FromPublicApi(rd);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "RestartPartitionDescription.FromPublicApi failed with {0}", error);
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    auto inner = client.TestMgmtClient->BeginRestartPartition(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnStartPartitionRestartComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnStartPartitionRestartComplete(inner, true);
}

void ApplicationsHandler::OnStartPartitionRestartComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndRestartPartition(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsHandler::GetServiceName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid partitionId;
    auto error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetServiceName(
        partitionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetServiceNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetServiceNameComplete(operation, true);
}

void ApplicationsHandler::OnGetServiceNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ServiceNameQueryResult result;
    auto error = client.QueryClient->EndGetServiceName(operation, result);
    bool useDelimiter = false;
    if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
    {
        useDelimiter = true;
    }

    ServiceNameQueryResultWrapper wrapper(move(result), useDelimiter);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(wrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetApplicationName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationName(
        serviceNameUri,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetApplicationNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetApplicationNameComplete(operation, true);
}

void ApplicationsHandler::OnGetApplicationNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ApplicationNameQueryResult result;
    auto error = client.QueryClient->EndGetApplicationName(operation, result);
    bool useDelimiter = false;
    if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
    {
        useDelimiter = true;
    }

    ApplicationNameQueryResultWrapper wrapper(move(result), useDelimiter);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(wrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsHandler::GetApplicationLoadInformation(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationLoadInformation(
        appNameUri.ToString(),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->GetApplicationLoadInformationComplete(operation, false);
    },
        thisSPtr);

    GetApplicationLoadInformationComplete(operation, true);
}

void ApplicationsHandler::GetApplicationLoadInformationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ApplicationLoadInformationQueryResult queryResult;
    auto error = client.QueryClient->EndGetApplicationLoadInformation(operation, queryResult);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(queryResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

#if !defined(PLATFORM_UNIX)
void ApplicationsHandler::PerformBackupRestoreOperation(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    // headers that will be sent to the Backup Restore System service.
    unordered_map<wstring, wstring> headersToAdd;

    // Compute X-Request-Metadata header value: URL Suffix upto the BackupRestoreOperation segment that needs to be forwarded to the service.
    StringCollection uriElements;
    wstring metadataHeaderVal;
    StringUtility::Split<wstring>(handlerOperation->MessageContext.GetSuffix(), uriElements, *HttpCommon::HttpConstants::QueryStringDelimiter, false);
    auto found = uriElements[0].find_last_of(Constants::MetadataSegment);
    if (found != wstring::npos)
    {
        metadataHeaderVal = uriElements[0].substr(0, found - (*Constants::MetadataSegment).size() + 1);
    }
    else
    {
        metadataHeaderVal = uriElements[0];
    }

    pair<wstring, wstring> urlSuffixHeader(Constants::RequestMetadataHeaderName, move(metadataHeaderVal));
    headersToAdd.insert(move(urlSuffixHeader));

    // X-Role header that will be sent to Backup Restore service indicating the client's role for authorization
    pair<wstring, wstring>  roleHeader(Constants::ClientRoleHeaderName, Transport::RoleMask::EnumToString(handlerOperation->FabricClient.ClientRole));
    headersToAdd.insert(move(roleHeader));

    handlerOperation->SetAdditionalHeadersToSend(move(headersToAdd));
    handlerOperation->SetServiceName(Constants::BackupRestoreServiceName);

    AsyncOperationSPtr operation = server_.AppGatewayHandler->BeginProcessReverseProxyRequest(
        Common::Guid::NewGuid().ToString(), // New trace Id for tracking the request end to end
        handlerOperation->MessageContextUPtr,
        handlerOperation->TakeBody(),
        wstring(),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnPerformBackupRestoreOperationComplete(operation, false);
    },
        thisSPtr);

    OnPerformBackupRestoreOperationComplete(operation, true);
}

void ApplicationsHandler::OnPerformBackupRestoreOperationComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ErrorCodeValue::Success;
    error = server_.AppGatewayHandler->EndProcessReverseProxyRequest(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "PerformBackupRestoreOperation EndProcessRequest failed with {0}", error);
    }

    // Complete the async.
    // AppGatewayHandler ProcessReverseProxyRequest would have responded to the HTTP request.
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    handlerOperation->TryComplete(operation->Parent, error);
}
#endif

void ApplicationsHandler::GetAllApplicationNetworks(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ApplicationNetworkQueryDescription applicationNetworkQueryDescription;

    NamingUri applicationName;
    auto error = argumentParser.TryGetApplicationName(applicationName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    applicationNetworkQueryDescription.ApplicationName = move(applicationName);

    QueryPagingDescription pagingDescription;
    error = argumentParser.TryGetPagingDescription(pagingDescription);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    applicationNetworkQueryDescription.QueryPagingDescriptionObject = make_unique<QueryPagingDescription>(move(pagingDescription));

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetApplicationNetworkList(
        move(applicationNetworkQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllApplicationNetworksComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetAllApplicationNetworksComplete(operation, true);
}

void ApplicationsHandler::OnGetAllApplicationNetworksComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ApplicationNetworkQueryResult> applicationNetworks;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetApplicationNetworkList(operation, applicationNetworks, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ApplicationNetworkList applicationNetworkList;
    if (pagingStatus)
    {
        applicationNetworkList.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    applicationNetworkList.Items = move(applicationNetworks);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(applicationNetworkList, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}