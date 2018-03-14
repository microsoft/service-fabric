// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;

ReadWriteStatusValidator::ReadWriteStatusValidator(Common::ComPointer<IFabricStatefulServicePartition> const& partition)
    : partition_(partition)
{
}

void ReadWriteStatusValidator::OnOpen()
{
    ValidateStatusIsNotPrimaryOrTryAgain(L"Open");
}

void ReadWriteStatusValidator::OnClose()
{
    ValidateStatusIsNotPrimary(L"Close");
}

void ReadWriteStatusValidator::OnAbort()
{
    ValidateStatusIsNotPrimary(L"Abort");
}

void ReadWriteStatusValidator::OnChangeRole(FABRIC_REPLICA_ROLE newRole)
{
    if (newRole == FABRIC_REPLICA_ROLE_PRIMARY)
    {
        ValidateStatusIsTryAgain(L"ChangeRolePrimary");
    }
    else if (newRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        ValidateStatusIsNotPrimaryOrTryAgain(L"ChangeRoleSecondary");
    }
    else
    {
        ValidateStatusIsNotPrimary(L"ChnageRoleOther");
    }
}

void ReadWriteStatusValidator::OnOnDataLoss()
{
    ValidateStatusIsTryAgain(L"OnDataLoss");
}

void ReadWriteStatusValidator::ValidateStatusIsNotPrimary(
    std::wstring const & tag)
{
    ValidateStatusIs(tag, FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
}

void ReadWriteStatusValidator::ValidateStatusIsTryAgain(
    std::wstring const & tag)
{
    ValidateStatusIs(tag, FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
}

void ReadWriteStatusValidator::ValidateStatusIsNotPrimaryOrTryAgain(
    std::wstring const & tag)
{
    ValidateStatusIs(tag, FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY, FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
}

void ReadWriteStatusValidator::ValidateStatusIs(
    std::wstring const & tag,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS expected1)
{
    ValidateStatusIs(StatusType::Read, tag, &expected1, nullptr);
    ValidateStatusIs(StatusType::Write, tag, &expected1, nullptr);
}

void ReadWriteStatusValidator::ValidateStatusIs(
    std::wstring const & tag,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS expected1,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS expected2)
{
    ValidateStatusIs(StatusType::Read, tag, &expected1, &expected2);
    ValidateStatusIs(StatusType::Write, tag, &expected1, &expected2);
}

void ReadWriteStatusValidator::ValidateStatusIs(
    StatusType::Enum type,
    std::wstring const & tag,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS * e1,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS * e2)
{
    vector<FABRIC_SERVICE_PARTITION_ACCESS_STATUS> expected;
    if (e1 != nullptr) expected.push_back(*e1);
    if (e2 != nullptr) expected.push_back(*e2);

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS actual = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
    if (!TryGetStatus(type, actual))
    {
        return;
    }

    if (find(expected.cbegin(), expected.cend(), actual) != expected.cend())
    {
        return;
    }

    // At this time the status is not in the expected list
    // Check it the lease is still valid
    auto typeText = type == StatusType::Read ? "read" : "write";
    bool isLeaseExpired = false;
    auto error = ReliabilityTestApi::ReconfigurationAgentComponentTestApi::IsLeaseExpired(partition_, isLeaseExpired);
    TestSession::WriteNoise("RWStatusValidator", "IsLeaseExpired {0}. Error {1}. Actual {2}, Type {3}", isLeaseExpired, error, actual, typeText);
    if (!error.IsSuccess())
    {
        return;
    }

    if (isLeaseExpired)
    {
        TestSession::FailTestIf(actual != FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY && actual != FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING, "{0} status at {1} is {2} when lease is expired. Expected FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY or FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING",
            typeText,
            tag,
            actual);
        return;
    }

    wstring allowed;
    for (auto const & it : expected)
    {
        allowed += wformatString(it) + L" ";
    }

    TestSession::FailTest("{0} status mismatch at {1}. Actual = {2}. Allowed = [{3}]",
        type == StatusType::Read ? "read" : "write",
        tag,
        actual,
        allowed);
}

bool ReadWriteStatusValidator::TryGetStatus(
    StatusType::Enum type,
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS & status)
{
    function<HRESULT(FABRIC_SERVICE_PARTITION_ACCESS_STATUS &)> func;

    if (type == StatusType::Read)
    {
        func = [this](FABRIC_SERVICE_PARTITION_ACCESS_STATUS & inner) { return partition_->GetReadStatus(&inner); };
    }
    else
    {
        func = [this](FABRIC_SERVICE_PARTITION_ACCESS_STATUS & inner) { return partition_->GetWriteStatus(&inner); };
    }

    auto hr = func(status);
    if (SUCCEEDED(hr))
    {
        TestSession::FailTestIf(status == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID, "Cannot return invalid partition access status");
        return true;
    }
    else if (hr == FABRIC_E_OBJECT_CLOSED)
    {
        return false;
    }
    else
    {
        TestSession::FailTest("Unknown return value {0}", hr);
        return false;
    }
}
