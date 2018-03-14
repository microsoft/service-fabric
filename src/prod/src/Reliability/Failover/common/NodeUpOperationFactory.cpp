// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ServiceModel;

namespace
{
    StringLiteral const TraceLifeCycle("LifeCycle");
}

NodeUpOperationFactory::NodeUpOperationFactory()
{
}

NodeUpOperationFactory::NodeUpOperationFactory(NodeUpOperationFactory && other)
: fmData_(move(other.fmData_)),
  fmmData_(move(other.fmmData_))
{
}

NodeUpOperationFactory & NodeUpOperationFactory::operator=(NodeUpOperationFactory && other)
{
    if (this != &other)
    {
        fmData_ = move(other.fmData_);
        fmmData_ = move(other.fmmData_);
    }

    return *this;
}

void NodeUpOperationFactory::AddClosedFailoverUnitFromLfumLoad(FailoverUnitId const & ftId)
{
    UpdateAnyReplicaFound(ftId);
}

void NodeUpOperationFactory::AddOpenFailoverUnitFromLfumLoad(
    FailoverUnitId const & ftId, 
    ServicePackageIdentifier const& packageId, 
    ServicePackageVersionInstance const & servicePackageVersionInstance)
{
    UpdateAnyReplicaFound(ftId);

    AddPackageToList(ftId, packageId, servicePackageVersionInstance);
}

void NodeUpOperationFactory::UpdateAnyReplicaFound(FailoverUnitId const & ftId)
{
    // For FMM we need to report any replica found as true if there is anything on the node
    // For FM we need to report any replica found only for replicas that the FM manages
    // and FM does not manage FM service replica (FMM manages that)
    if (ftId.IsFM)
    {
        fmmData_.MarkAnyReplicaFound();
    }
    else
    {
        fmmData_.MarkAnyReplicaFound();
        fmData_.MarkAnyReplicaFound();
    }
}

void NodeUpOperationFactory::CreateAndSendNodeUpToFmm(IReliabilitySubsystem & reliability) const
{
    CreateAndSendNodeUp(true, fmmData_, reliability);
}

void NodeUpOperationFactory::CreateAndSendNodeUpToFM(IReliabilitySubsystem & reliability) const
{
    CreateAndSendNodeUp(false, fmData_, reliability);
}

void NodeUpOperationFactory::AddPackageToList(
    FailoverUnitId const & ftId, 
    ServicePackageIdentifier const& packageId, 
    ServicePackageVersionInstance const & servicePackageVersionInstance)
{
    // The FMM list is always empty
    // The FM list never contains the FM replica
    if (ftId.IsFM)
    {
        return;
    }

    fmData_.AddServicePackage(packageId, servicePackageVersionInstance);
}

void NodeUpOperationFactory::CreateAndSendNodeUp(bool isFmm, NodeUpData const & data, IReliabilitySubsystem & reliability)
{
    vector<ServicePackageInfo> packages = data.GetServicePackages();
    bool anyReplicaFound = data.AnyReplicaFound;

    WriteInfo(
        TraceLifeCycle, 
        reliability.FederationWrapperBase.InstanceIdStr, 
        "Sending NodeUp message to {0} from node {1}. Current version = {2}. Packages: {3}. Any Replica Found = {4}", 
        isFmm ? "FMM" : "FM",
        reliability.FederationWrapperBase.Instance, 
        reliability.NodeConfig->NodeVersion, 
        packages, 
        anyReplicaFound);

    NodeUpOperationSPtr nodeUp = NodeUpOperation::Create(reliability, move(packages), isFmm, anyReplicaFound);
    nodeUp->Send();
}

NodeUpOperationFactory::NodeUpData::NodeUpData()
: anyReplicaFound_(false)
{
}

NodeUpOperationFactory::NodeUpData::NodeUpData(NodeUpData && other)
: anyReplicaFound_(move(other.anyReplicaFound_)),
  servicePackages_(move(other.servicePackages_))
{
}

NodeUpOperationFactory::NodeUpData & NodeUpOperationFactory::NodeUpData::operator=(NodeUpOperationFactory::NodeUpData && other)
{
    if (this != &other)
    {
        anyReplicaFound_ = move(other.anyReplicaFound_);
        servicePackages_ = move(other.servicePackages_);
    }

    return *this;
}

void NodeUpOperationFactory::NodeUpData::MarkAnyReplicaFound()
{
    AcquireWriteLock grab(lock_);
    anyReplicaFound_ = true;
}

vector<ServicePackageInfo> NodeUpOperationFactory::NodeUpData::GetServicePackages() const
{
    AcquireReadLock grab(lock_);
    // Necessary to make a copy because multiple node up can be sent
    return servicePackages_;
}

void NodeUpOperationFactory::NodeUpData::AddServicePackage(
    ServicePackageIdentifier const& packageId, 
    ServicePackageVersionInstance const & servicePackageVersionInstance)
{
    AcquireWriteLock grab(lock_);
    auto it = find_if(
        servicePackages_.begin(),
        servicePackages_.end(),
        [&packageId] (ServicePackageInfo const& package) { return packageId == package.PackageId; });

    /*
        During application upgrade the version on the FTs being impacted is updated
        one at a time and not under a single transaction

        This means that if the node fails while part of the FTs are updated then there
        is an inconsistency in the package version instance across all the FTs for that app

        During node up, the node should report the lowest package version instance it finds

        This will cause the FM to send the correct package version instance for the service package
        in the node up ack which will cause all the FTs to be updated to the correct package version instance
    */
    if (it == servicePackages_.end())
    {
        ServicePackageInfo package(packageId, servicePackageVersionInstance);
        servicePackages_.push_back(move(package));
    }    
    else if (it->VersionInstance.InstanceId > servicePackageVersionInstance.InstanceId)
    {
        ServicePackageInfo package(packageId, servicePackageVersionInstance);
        *it = move(package);
    }
}
