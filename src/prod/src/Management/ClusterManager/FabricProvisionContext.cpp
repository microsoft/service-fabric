// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("FabricProvisionContext");

RolloutContextType::Enum const FabricProvisionContextType(RolloutContextType::FabricProvision);

FabricProvisionContext::FabricProvisionContext()
    : RolloutContext(FabricProvisionContextType)
    , codeFilepath_()
    , clusterManifestFilepath_()
    , version_()
    , isProvision_(false)
    , provisionedCodeVersions_()
    , provisionedConfigVersions_()
{
}

FabricProvisionContext::FabricProvisionContext(
    FabricProvisionContext && other)
    : RolloutContext(move(other))
    , codeFilepath_(move(other.codeFilepath_))
    , clusterManifestFilepath_(move(other.clusterManifestFilepath_))
    , version_(move(other.version_))
    , isProvision_(move(other.isProvision_))
    , provisionedCodeVersions_(move(other.provisionedCodeVersions_))
    , provisionedConfigVersions_(move(other.provisionedConfigVersions_))
{
}

FabricProvisionContext & FabricProvisionContext::operator=(
    FabricProvisionContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        codeFilepath_ = move(other.codeFilepath_);
        clusterManifestFilepath_ = move(other.clusterManifestFilepath_);
        version_ = move(other.version_);
        isProvision_ = move(other.isProvision_);
        provisionedCodeVersions_ = move(other.provisionedCodeVersions_);
        provisionedConfigVersions_ = move(other.provisionedConfigVersions_);
    }
    
    return *this;
}

FabricProvisionContext::FabricProvisionContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    std::wstring const & codeFilepath,
    std::wstring const & clusterManifestFilepath,
    Common::FabricVersion const & fabricVersion)
    : RolloutContext(FabricProvisionContextType, replica, request)
    , codeFilepath_(codeFilepath)
    , clusterManifestFilepath_(clusterManifestFilepath)
    , version_(fabricVersion)
    , isProvision_(true)
    , provisionedCodeVersions_()
    , provisionedConfigVersions_()
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1}, {2}, {3}, isProvision={4})",
        this->TraceId,
        codeFilepath_,
        clusterManifestFilepath_,
        version_,
        isProvision_);
}

FabricProvisionContext::FabricProvisionContext(
    ComponentRoot const &,
    Store::ReplicaActivityId const & activityId,
    std::wstring const & codeFilepath,
    std::wstring const & clusterManifestFilepath,
    Common::FabricVersion const & fabricVersion)
    : RolloutContext(FabricProvisionContextType, activityId)
    , codeFilepath_(codeFilepath)
    , clusterManifestFilepath_(clusterManifestFilepath)
    , version_(fabricVersion)
    , isProvision_(true)
    , provisionedCodeVersions_()
    , provisionedConfigVersions_()
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1}, {2}, {3}, isProvision={4})",
        this->TraceId,
        codeFilepath_,
        clusterManifestFilepath_,
        version_,
        isProvision_);
}

FabricProvisionContext::~FabricProvisionContext()
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

bool FabricProvisionContext::ContainsCodeVersion(Common::FabricVersion const & fabricVersion)
{
    return (provisionedCodeVersions_.find(fabricVersion.CodeVersion) != provisionedCodeVersions_.end());
}

bool FabricProvisionContext::ContainsConfigVersion(Common::FabricVersion const & fabricVersion)
{
    return (provisionedConfigVersions_.find(fabricVersion.ConfigVersion) != provisionedConfigVersions_.end());
}

std::wstring FabricProvisionContext::FormatProvisionedCodeVersions() const
{
    wstring result = L"{ ";
    StringWriter writer(result);
    for (auto it = provisionedCodeVersions_.begin(); it != provisionedCodeVersions_.end(); ++it)
    {
        writer.Write("{0} ", *it);
    }
    writer.Write(L"}");

    return result;
}

std::wstring FabricProvisionContext::FormatProvisionedConfigVersions() const
{
    wstring result = L"{ ";
    StringWriter writer(result);
    for (auto it = provisionedConfigVersions_.begin(); it != provisionedConfigVersions_.end(); ++it)
    {
        writer.Write("{0} ", *it);
    }
    writer.Write(L"}");

    return result;
}

ErrorCode FabricProvisionContext::StartProvisioning(
    StoreTransaction const & storeTx,
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    std::wstring const & codeFilepath, 
    std::wstring const & clusterManifestFilepath, 
    Common::FabricVersion const & fabricVersion)
{
    codeFilepath_ = codeFilepath;
    clusterManifestFilepath_ = clusterManifestFilepath;
    version_ = fabricVersion;
    isProvision_ = true;

    this->ReInitializeContext(replica, request);

    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode FabricProvisionContext::StartUnprovisioning(
    StoreTransaction const & storeTx,
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    Common::FabricVersion const & fabricVersion)
{
    codeFilepath_.clear();
    clusterManifestFilepath_.clear();
    version_ = fabricVersion;
    isProvision_ = false;

    this->ReInitializeContext(replica, request);

    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode FabricProvisionContext::CompleteProvisioning(StoreTransaction const & storeTx)
{
    if (version_.CodeVersion.IsValid)
    {
        provisionedCodeVersions_.insert(version_.CodeVersion);
    }

    if (version_.ConfigVersion.IsValid)
    {
        provisionedConfigVersions_.insert(version_.ConfigVersion);
    }

    return this->Complete(storeTx);
}

ErrorCode FabricProvisionContext::CompleteUnprovisioning(StoreTransaction const & storeTx)
{
    if (version_.CodeVersion.IsValid)
    {
        provisionedCodeVersions_.erase(version_.CodeVersion);
    }

    if (version_.ConfigVersion.IsValid)
    {
        provisionedConfigVersions_.erase(version_.ConfigVersion);
    }

    return this->Complete(storeTx);
}

std::wstring const & FabricProvisionContext::get_Type() const
{
    return Constants::StoreType_FabricProvisionContext;
}

std::wstring FabricProvisionContext::ConstructKey() const
{
    return Constants::StoreKey_FabricProvisionContext;
}

void FabricProvisionContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "FabricProvisionContext({0})[{1}, {2}, {3}, isProvision={4}]", 
        this->Status, 
        codeFilepath_, 
        clusterManifestFilepath_, 
        version_, 
        isProvision_);
}
