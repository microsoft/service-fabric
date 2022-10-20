// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationTypeContext");

RolloutContextType::Enum const ApplicationTypeContextType(RolloutContextType::ApplicationType);

ApplicationTypeContext::ApplicationTypeContext()
    : RolloutContext(ApplicationTypeContextType)
    , buildPath_()
    , typeName_()
    , typeVersion_()
    , manifestDataInStore_(false)
    , isAsync_(false)
    , statusDetails_()
    , applicationTypeDefinitionKind_(ApplicationTypeDefinitionKind::Enum::ServiceFabricApplicationPackage)
    , manifestId_()
    , applicationPackageDownloadUri_()
    , applicationPackageCleanupPolicy_(ApplicationPackageCleanupPolicy::Enum::Invalid)
{
}

ApplicationTypeContext::ApplicationTypeContext(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion)
    : RolloutContext(ApplicationTypeContextType)
    , buildPath_()
    , typeName_(typeName)
    , typeVersion_(typeVersion)
    , manifestDataInStore_(false)
    , isAsync_(false)
    , statusDetails_()
    , applicationTypeDefinitionKind_(ApplicationTypeDefinitionKind::Enum::ServiceFabricApplicationPackage)
    , manifestId_()
    , applicationPackageDownloadUri_()
    , applicationPackageCleanupPolicy_(ApplicationPackageCleanupPolicy::Enum::Invalid)
{
}

ApplicationTypeContext::ApplicationTypeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    std::wstring const & buildPath,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    bool isAsync,
    std::wstring const & applicationPackageDownloadUri)
    : RolloutContext(ApplicationTypeContextType, replica, request)
    , buildPath_(buildPath)
    , typeName_(typeName)
    , typeVersion_(typeVersion)
    , manifestDataInStore_(false)
    , isAsync_(isAsync)
    , statusDetails_()
    , applicationTypeDefinitionKind_(ApplicationTypeDefinitionKind::Enum::ServiceFabricApplicationPackage)
    , manifestId_()
    , applicationPackageDownloadUri_(applicationPackageDownloadUri)
    , applicationPackageCleanupPolicy_(ApplicationPackageCleanupPolicy::Enum::Invalid)
{
}

ApplicationTypeContext::ApplicationTypeContext(
    Store::ReplicaActivityId const &replicaActivityId,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &typeVersion,
    bool isAsync,
    ApplicationTypeDefinitionKind::Enum const definitionKind,
    std::wstring const & manifestId)
    : RolloutContext(ApplicationTypeContextType, replicaActivityId)
    , typeName_(typeName)
    , typeVersion_(typeVersion)
    , manifestDataInStore_(false)
    , isAsync_(isAsync)
    , statusDetails_()
    , applicationTypeDefinitionKind_(definitionKind)
    , manifestId_(manifestId)
    , applicationPackageDownloadUri_()
    , applicationPackageCleanupPolicy_(ApplicationPackageCleanupPolicy::Enum::Invalid)
{
}

ApplicationTypeContext::ApplicationTypeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    std::wstring const & buildPath,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    bool isAsync,
    std::wstring const & applicationPackageDownloadUri,
    ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy)
    : RolloutContext(ApplicationTypeContextType, replica, request)
    , buildPath_(buildPath)
    , typeName_(typeName)
    , typeVersion_(typeVersion)
    , manifestDataInStore_(false)
    , isAsync_(isAsync)
    , statusDetails_()
    , applicationTypeDefinitionKind_(ApplicationTypeDefinitionKind::Enum::ServiceFabricApplicationPackage)
    , manifestId_()
    , applicationPackageDownloadUri_(applicationPackageDownloadUri)
    , applicationPackageCleanupPolicy_(applicationPackageCleanupPolicy)
{
}

ApplicationTypeContext::~ApplicationTypeContext()
{
    // Will be null for contexts instantiated for reading from disk.
    // Do not trace dtor for these contexts as it is noisy and not useful.
    //
    if (replicaSPtr_)
    {
        WriteNoise(
            TraceComponent,
            "{0} ~dtor",
            this->TraceId);
    }
}

std::wstring const & ApplicationTypeContext::get_Type() const
{
    return Constants::StoreType_ApplicationTypeContext;
}

std::wstring ApplicationTypeContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        this->GetTypeNameKeyPrefix(),
        typeVersion_);
    return temp;
}

std::wstring ApplicationTypeContext::GetTypeNameKeyPrefix() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        typeName_,
        Constants::TokenDelimeter);
    return temp;
}

ApplicationTypeStatus::Enum ApplicationTypeContext::get_QueryStatus() const
{
    switch (this->Status)
    {
    case RolloutStatus::Pending:
        return ApplicationTypeStatus::Provisioning;
    case RolloutStatus::DeletePending:
        return ApplicationTypeStatus::Unprovisioning;
    case RolloutStatus::Completed:
        return ApplicationTypeStatus::Available;
    case RolloutStatus::Failed:
        return ApplicationTypeStatus::Failed;
    default:
        return ApplicationTypeStatus::Invalid;
    }
}

void ApplicationTypeContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ApplicationTypeContext({0})[{1}, {2}, {3}, async={4}, definitionKind={5}, manifestId={6}, downloadPath='{7}', cleanupPolicy={8}]",
        this->Status, buildPath_, typeName_, typeVersion_, isAsync_, applicationTypeDefinitionKind_, manifestId_, applicationPackageDownloadUri_, (int) applicationPackageCleanupPolicy_);
}
