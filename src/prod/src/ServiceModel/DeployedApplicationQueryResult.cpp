// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(DeployedApplicationQueryResult)

DeployedApplicationQueryResult::DeployedApplicationQueryResult()
    : applicationName_()
    , applicationTypeName_()
    , deployedApplicationStatus_(DeploymentStatus::Invalid)
    , workDirectory_()
    , logDirectory_()
    , tempDirectory_()
    , healthState_(FABRIC_HEALTH_STATE_UNKNOWN)
{
}

DeployedApplicationQueryResult::DeployedApplicationQueryResult(
    std::wstring const & applicationName,
    std::wstring const & applicationTypeName,
    DeploymentStatus::Enum deployedApplicationStatus,
    wstring const & workDirectory,
    wstring const & logDirectory,
    wstring const & tempDirectory,
    FABRIC_HEALTH_STATE healthState)
    : applicationName_(applicationName)
    , applicationTypeName_(applicationTypeName)
    , deployedApplicationStatus_(deployedApplicationStatus)
    , workDirectory_(workDirectory)
    , logDirectory_(logDirectory)
    , tempDirectory_(tempDirectory)
    , healthState_(healthState)
{
}

void DeployedApplicationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM & publicDeployedApplicationQueryResult) const
{
    publicDeployedApplicationQueryResult.ApplicationName = heap.AddString(applicationName_);
    publicDeployedApplicationQueryResult.ApplicationTypeName = heap.AddString(applicationTypeName_);
    publicDeployedApplicationQueryResult.DeployedApplicationStatus = DeploymentStatus::ToPublicApi(deployedApplicationStatus_);

    auto publicDeployedApplicationQueryResultEx = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX>();
    publicDeployedApplicationQueryResultEx->WorkDirectory = heap.AddString(workDirectory_, true);
    publicDeployedApplicationQueryResultEx->LogDirectory = heap.AddString(logDirectory_, true);
    publicDeployedApplicationQueryResultEx->TempDirectory = heap.AddString(tempDirectory_, true);

    auto publicDeployedApplicationQueryResultEx2 = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX2>();
    publicDeployedApplicationQueryResultEx2->HealthState = healthState_;
    publicDeployedApplicationQueryResultEx->Reserved = publicDeployedApplicationQueryResultEx2.GetRawPointer();

    publicDeployedApplicationQueryResult.Reserved = publicDeployedApplicationQueryResultEx.GetRawPointer();
}

void DeployedApplicationQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("DeployedApplicationQueryResult { ");
    w.Write("ApplicationName = {0}, ", this->ApplicationName);
    w.Write("ApplicationTypeName = {0}, ", this->ApplicationTypeName);
    w.Write("DeployedApplicationStatus = {0}, ", this->DeployedApplicationStatus);
    w.Write("WorkDirectory = {0}, ", this->WorkDirectory);
    w.Write("LogDirectory = {0}, ", this->LogDirectory);
    w.Write("TempDirectory = {0}, ", this->TempDirectory);
    w.Write("HealthState = {0}, ", this->HealthState);
    w.Write("}");
}

std::wstring DeployedApplicationQueryResult::ToString() const
{
    return wformatString(*this);
}

Common::ErrorCode DeployedApplicationQueryResult::FromPublicApi(__in FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM const & publicDeployedApplicationQueryResult)
{
    auto error = StringUtility::LpcwstrToWstring2(publicDeployedApplicationQueryResult.ApplicationName, false, applicationName_);
    if (!error.IsSuccess()) { return error; }

    error = StringUtility::LpcwstrToWstring2(publicDeployedApplicationQueryResult.ApplicationTypeName, false, applicationTypeName_);
    if (!error.IsSuccess()) { return error; }

    error = DeploymentStatus::FromPublicApi(publicDeployedApplicationQueryResult.DeployedApplicationStatus, deployedApplicationStatus_);
    if (!error.IsSuccess()) { return error; }

    // Ex1
    if (publicDeployedApplicationQueryResult.Reserved != NULL)
    {
        auto publicDeployedApplicationQueryResultEx1 =
            reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX *>(publicDeployedApplicationQueryResult.Reserved);

        error = StringUtility::LpcwstrToWstring2(publicDeployedApplicationQueryResultEx1->WorkDirectory, true, workDirectory_);
        if (!error.IsSuccess()) { return error; }

        error = StringUtility::LpcwstrToWstring2(publicDeployedApplicationQueryResultEx1->LogDirectory, true, logDirectory_);
        if (!error.IsSuccess()) { return error; }

        error = StringUtility::LpcwstrToWstring2(publicDeployedApplicationQueryResultEx1->TempDirectory, true, tempDirectory_);
        if (!error.IsSuccess()) { return error; }

        // Ex2
        if (publicDeployedApplicationQueryResultEx1->Reserved != NULL)
        {
            auto publicDeployedApplicationQueryResultEx2 =
                reinterpret_cast<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX2 *>(publicDeployedApplicationQueryResultEx1->Reserved);

            healthState_ = publicDeployedApplicationQueryResultEx2->HealthState;
        }
    }

    return ErrorCodeValue::Success;
}

std::wstring DeployedApplicationQueryResult::CreateContinuationToken() const
{
    return applicationName_;
}
