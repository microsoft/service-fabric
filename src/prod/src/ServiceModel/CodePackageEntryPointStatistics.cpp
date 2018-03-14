// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

CodePackageEntryPointStatistics::CodePackageEntryPointStatistics()
    : LastExitCode(0),
    LastActivationTime(DateTime::Zero),
    LastSuccessfulActivationTime(DateTime::Zero),
    LastExitTime(DateTime::Zero),
    LastSuccessfulExitTime(DateTime::Zero),
    ActivationFailureCount(0),
    ContinuousActivationFailureCount(0),
    ExitFailureCount(0),
    ContinuousExitFailureCount(0),
    ExitCount(0),
    ActivationCount(0)
{
}

void CodePackageEntryPointStatistics::ToPublicApi(
    __in Common::ScopedHeap & , 
    __out FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS & publicCodePackageEntryPointStatistics) const 
{
    
    publicCodePackageEntryPointStatistics.LastExitCode = LastExitCode;
    publicCodePackageEntryPointStatistics.LastActivationUtc = LastActivationTime.AsFileTime;
    publicCodePackageEntryPointStatistics.LastExitUtc = LastExitTime.AsFileTime;
    publicCodePackageEntryPointStatistics.LastSuccessfulActivationUtc = LastSuccessfulActivationTime.AsFileTime;
    publicCodePackageEntryPointStatistics.LastSuccessfulExitUtc = LastSuccessfulExitTime.AsFileTime;
    publicCodePackageEntryPointStatistics.ActivationCount = ActivationCount;
    publicCodePackageEntryPointStatistics.ActivationFailureCount = ActivationFailureCount;
    publicCodePackageEntryPointStatistics.ContinuousActivationFailureCount = ContinuousActivationFailureCount;
    publicCodePackageEntryPointStatistics.ExitCount = ExitCount;
    publicCodePackageEntryPointStatistics.ExitFailureCount = ExitFailureCount;
    publicCodePackageEntryPointStatistics.ContinuousExitFailureCount = ContinuousExitFailureCount;
}

void CodePackageEntryPointStatistics::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

wstring CodePackageEntryPointStatistics::ToString() const
{
    return wformatString(
        "LastExitCode = {0}, LastActivationTime = {1}, LastExitTime = {2}, LastSuccessfulActivationTime = {3}, LastSuccessfulExitTime = {4}, ActivationCount = {5}, ActivationFailureCount = {6}, ContinuousActivationFailureCount = {7}, {8}",
        LastExitCode,
        LastActivationTime,
        LastExitCode,
        LastSuccessfulActivationTime,
        LastSuccessfulExitTime,
        ActivationCount,
        ActivationFailureCount,
        ContinuousActivationFailureCount,
        wformatString("ExitCount = {0},  ExitFailureCount = {1}, ContinuousExitFailureCount = {2}", ExitCount, ExitFailureCount, ContinuousExitFailureCount));
}

ErrorCode CodePackageEntryPointStatistics::FromPublicApi(__in FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS const & publicCodePackageEntryPointStatistics)
{
    LastExitCode = publicCodePackageEntryPointStatistics.LastExitCode;
    LastActivationTime = Common::DateTime(publicCodePackageEntryPointStatistics.LastActivationUtc);
    LastExitTime = Common::DateTime(publicCodePackageEntryPointStatistics.LastExitUtc);
    LastSuccessfulActivationTime = Common::DateTime(publicCodePackageEntryPointStatistics.LastSuccessfulActivationUtc);
    LastSuccessfulExitTime = Common::DateTime(publicCodePackageEntryPointStatistics.LastSuccessfulExitUtc);
    ActivationCount = publicCodePackageEntryPointStatistics.ActivationCount;
    ActivationFailureCount = publicCodePackageEntryPointStatistics.ActivationFailureCount;
    ContinuousActivationFailureCount = publicCodePackageEntryPointStatistics.ContinuousActivationFailureCount;
    ExitCount = publicCodePackageEntryPointStatistics.ExitCount;
    ExitFailureCount = publicCodePackageEntryPointStatistics.ExitFailureCount;
    ContinuousExitFailureCount = publicCodePackageEntryPointStatistics.ContinuousExitFailureCount;    
    return ErrorCode::Success();
}

