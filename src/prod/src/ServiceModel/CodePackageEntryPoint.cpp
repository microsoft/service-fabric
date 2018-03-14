// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

CodePackageEntryPoint::CodePackageEntryPoint()
    : entryPointLocation_()
    , processId_()
    , runAsUserName_()
    , nextActivationTime_(DateTime::Zero)
    , entryPointStatus_(EntryPointStatus::Invalid)
    , statistics_()
    , instanceId_(0)
{
}

CodePackageEntryPoint::CodePackageEntryPoint(
    wstring const & entryPointLocation, 
    int64 processId, 
    wstring const & runAsUserName, 
    DateTime const & nextActivationTime,        
    EntryPointStatus::Enum entryPointStatus,
    CodePackageEntryPointStatistics const & statistics,
    int64 instanceId)
    : entryPointLocation_(entryPointLocation)
    , processId_(processId)
    , runAsUserName_(runAsUserName)
    , nextActivationTime_(nextActivationTime)
    , entryPointStatus_(entryPointStatus)
    , statistics_(statistics)
    , instanceId_(instanceId)
{
}

void CodePackageEntryPoint::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_CODE_PACKAGE_ENTRY_POINT & publicCodePackageEntryPoint) const
{
    publicCodePackageEntryPoint.EntryPointLocation = heap.AddString(entryPointLocation_);
    publicCodePackageEntryPoint.ProcessId = processId_;
    publicCodePackageEntryPoint.RunAsUserName = heap.AddString(runAsUserName_);
    publicCodePackageEntryPoint.NextActivationUtc = nextActivationTime_.AsFileTime;
    publicCodePackageEntryPoint.EntryPointStatus = EntryPointStatus::ToPublicApi(entryPointStatus_);

    auto publicCodePackageStatistics = heap.AddItem<FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS>();
    statistics_.ToPublicApi(heap, *publicCodePackageStatistics);    
    publicCodePackageEntryPoint.Statistics = publicCodePackageStatistics.GetRawPointer();

    auto publicCodePackageEntryEx1 = heap.AddItem<FABRIC_CODE_PACKAGE_ENTRY_POINT_EX1>();
    publicCodePackageEntryEx1->CodePackageInstanceId = instanceId_;

    publicCodePackageEntryPoint.Reserved = publicCodePackageEntryEx1.GetRawPointer();
}

void CodePackageEntryPoint::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring CodePackageEntryPoint::ToString() const
{
    return wformatString(
        "EntryPointLocation = {0}, ProcessId = {1}, RunAsUserName = {2}, NextActivationTime = {3}, Status = {4}, Statistics = [{5}], InstanceId = [{6}]",
        entryPointLocation_,
        processId_,
        runAsUserName_,
        nextActivationTime_,
        entryPointStatus_,
        statistics_.ToString(),
        instanceId_);
}

ErrorCode CodePackageEntryPoint::FromPublicApi(__in FABRIC_CODE_PACKAGE_ENTRY_POINT const & publicDeployedCodePackageQueryResult)
{
    entryPointLocation_ = wstring(publicDeployedCodePackageQueryResult.EntryPointLocation);
    processId_ = publicDeployedCodePackageQueryResult.ProcessId;
    runAsUserName_ = wstring(publicDeployedCodePackageQueryResult.RunAsUserName);
    nextActivationTime_ = DateTime(publicDeployedCodePackageQueryResult.NextActivationUtc);

    auto error = EntryPointStatus::FromPublicApi(publicDeployedCodePackageQueryResult.EntryPointStatus, entryPointStatus_);
    if(!error.IsSuccess()) { return error; }

    if (publicDeployedCodePackageQueryResult.Reserved != NULL)
    {
        FABRIC_CODE_PACKAGE_ENTRY_POINT_EX1 * publicCodePackageEntryResultEx1 = (FABRIC_CODE_PACKAGE_ENTRY_POINT_EX1*) publicDeployedCodePackageQueryResult.Reserved;
        instanceId_ = publicCodePackageEntryResultEx1->CodePackageInstanceId;
    }

    return statistics_.FromPublicApi(*publicDeployedCodePackageQueryResult.Statistics);
}
