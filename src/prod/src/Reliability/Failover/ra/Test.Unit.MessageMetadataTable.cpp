// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

using ReconfigurationAgentComponent::Communication::MessageMetadata;
using ReconfigurationAgentComponent::Communication::MessageMetadataTable;

class TestMessageMetadataTable 
{
protected:
    TestMessageMetadataTable();

    void ValidateMetadata(
        vector<wstring> const & allowedSet,
        function<bool(MessageMetadata const &)> allowedCheck) const;

    vector<wstring> AllMessages;
    MessageMetadataTable metadataTable_;
};

TestMessageMetadataTable::TestMessageMetadataTable()
{
    AllMessages = vector<wstring>
    {
        L"ServiceTypeEnabledReply",
        L"ServiceTypeDisabledReply",
        L"AddInstance",
        L"RemoveInstance",
        L"AddPrimary",
        L"DoReconfiguration",
        L"AddReplica",
        L"RemoveReplica",
        L"DeleteReplica",
        L"ServiceBusy",
        L"ChangeConfigurationReply",
        L"ReplicaUpReply",
        L"ReplicaEndpointUpdatedReply",
        L"CreateReplica",
        L"CreateReplicaReply",
        L"Deactivate",
        L"DeactivateReply",
        L"Activate",
        L"ActivateReply",
        L"GetLSN",
        L"GetLSNReply",
        L"GenerationUpdate",
        L"NodeFabricUpgradeRequest",
        L"NodeUpgradeRequest",
        L"CancelApplicationUpgradeRequest",
        L"CancelFabricUpgradeRequest",
        L"NodeUpdateServiceRequest",
        L"NodeDeactivateRequest",
        L"NodeActivateRequest",
        L"ClientReportFaultRequest",
        L"ServiceTypeNotificationReply",
        L"PartitionNotificationReply",

        L"RAReportFault",

        L"ReplicaOpenReply",
        L"ReplicaCloseReply",
        L"StatefulServiceReopenReply",
        L"UpdateConfigurationReply",
        L"ReplicatorBuildIdleReplicaReply",
        L"ReplicatorRemoveIdleReplicaReply",
        L"ReplicatorGetStatusReply",
        L"ReplicatorUpdateEpochAndGetStatusReply",
        L"CancelCatchupReplicaSetReply",

        L"ProxyReplicaEndpointUpdated",

        L"ReadWriteStatusRevokedNotification",

        L"ProxyUpdateServiceDescriptionReply",
    };
}

void TestMessageMetadataTable::ValidateMetadata(
    vector<wstring> const & allowedSet,
    function<bool(MessageMetadata const &)> checkFunc) const
{
    vector<wstring> failed;
    vector<wstring> metadataNotFound;

    for (auto const & i : AllMessages)
    {
        auto metadata = metadataTable_.LookupMetadata(i);

        if (metadata == nullptr)
        {
            metadataNotFound.push_back(i);
            continue;
        }

        bool expected = find(allowedSet.begin(), allowedSet.end(), i) != allowedSet.end();
        bool actual = checkFunc(*metadata);

        if (expected != actual)
        {
            failed.push_back(i);
        }
    }

    if (!failed.empty() || !metadataNotFound.empty())
    {
        wstring message;

        message += L"Failed\r\n";
        for (auto const & it : failed)
        {
            message += it;
            message += L"\r\n";
        }

        message += L"metadataNotFound\r\n";
        for (auto const & it : metadataNotFound)
        {
            message += it;
            message += L"\r\n";
        }

        Verify::Fail(message);
    }
}

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestMessageMetadataTableSuite, TestMessageMetadataTable)

BOOST_AUTO_TEST_CASE(TestProcessDuringNodeClose)
{
    vector<wstring> allowed = { L"ReplicaCloseReply", L"ReadWriteStatusRevokedNotification", L"ReplicaUpReply" };
    ValidateMetadata(
        allowed,
        [](MessageMetadata const & metadata) { return metadata.ProcessDuringNodeClose; });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
