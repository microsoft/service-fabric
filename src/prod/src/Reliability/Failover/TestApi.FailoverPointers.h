// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReliabilityTestApi
{
    class ReliabilityTestHelper;
        
    namespace FailoverManagerComponentTestApi
    {
        class FailoverManagerTestHelper;
        typedef std::unique_ptr<FailoverManagerTestHelper> FailoverManagerTestHelperUPtr;

        class FailoverUnitSnapshot;
        typedef std::unique_ptr<FailoverUnitSnapshot> FailoverUnitSnapshotUPtr;

        class ReplicaSnapshot;
        typedef std::unique_ptr<ReplicaSnapshot> ReplicaSnapshotUPtr;

        class NodeSnapshot;
        typedef std::unique_ptr<NodeSnapshot> NodeSnapshotUPtr;

        class ServiceSnapshot;
        typedef std::unique_ptr<ServiceSnapshot> ServiceSnapshotUPtr;

        class ApplicationSnapshot;
        typedef std::unique_ptr<ApplicationSnapshot> ApplicationSnapshotUPtr;

        class InBuildFailoverUnitSnapshot;
        class LoadSnapshot;
        class ServiceTypeSnapshot;
        class ApplicationSnapshot;
    }

    namespace ReconfigurationAgentComponentTestApi
    {
        class ReconfigurationAgentTestHelper;
        typedef std::unique_ptr<ReconfigurationAgentTestHelper> ReconfigurationAgentTestHelperUPtr;

        class FailoverUnitSnapshot;
        typedef std::unique_ptr<FailoverUnitSnapshot> FailoverUnitSnapshotUPtr;

        class ReplicaSnapshot;
        typedef std::unique_ptr<ReplicaSnapshot> ReplicaSnapshotUPtr;
        
        class ReconfigurationAgentProxyTestHelper;
        typedef std::unique_ptr<ReconfigurationAgentProxyTestHelper> ReconfigurationAgentProxyTestHelperUPtr;
    }
}

