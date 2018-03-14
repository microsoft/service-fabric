// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class IHealthAggregator
        {
        public:
            // Updates applications by applying the given reports.
            virtual Common::AsyncOperationSPtr BeginUpdateApplicationsHealth(
                Common::ActivityId const & activityId,
                std::vector<ServiceModel::HealthReport> && reports,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndUpdateApplicationsHealth(
                Common::AsyncOperationSPtr const & operation) = 0;

            // Updates the cluster health policies, including the upgrade health policy and application health policies.
            virtual Common::AsyncOperationSPtr BeginUpdateClusterHealthPolicy(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & updateHealthPolicy,
                ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) = 0;
            virtual Common::ErrorCode EndUpdateClusterHealthPolicy(
                Common::AsyncOperationSPtr const & operation) = 0;

            // Checks whether the application is healthy. 
            virtual Common::ErrorCode IsApplicationHealthy(
                Common::ActivityId const & activityId,
                std::wstring const & applicationName,
                // relevant UDs
                std::vector<std::wstring> const & upgradeDomains,
                ServiceModel::ApplicationHealthPolicy const * policy,
                __inout bool & result,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations) const = 0;

            // Creates a snapshot with global unhealthy percent as well as per UDs unhealthy percentages for nodes.
            // It looks through all UDs HM knows about.
            virtual Common::ErrorCode GetClusterUpgradeSnapshot(
                Common::ActivityId const & activityId,
                __inout ClusterUpgradeStateSnapshot & snapshot) const = 0;

            // If no upgrade domains are specified, returns global nodes max percent unhealthy 
            // and max percent unhealthy for all UDs HM knows about
            // Otherwise, returns only the max percent unhealthy for the specified upgrade domains.
            // Uses the ClusterHealthPolicy saved in the cluster attributes.
            virtual Common::ErrorCode AppendToClusterUpgradeSnapshot(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                __inout ClusterUpgradeStateSnapshot & snapshot) const = 0;

            // Evaluate cluster health.
            // Uses policies (clusterHealthPolicy and ClusterUpgradeHealthPolicy) saved in the cluster attributes.
            // If upgrade domains baseline is not empty, it computes per UD delta and checks against the baseline.
            // Otherwise, it checks per UD health relative to MaxPercentUnhealthyNodes.
            // If the applications do not have application type set and the per application type map from cluster health policy contains entries,
            // it replies back with Error to indicate required attributes are missing and returns a list of applications that need to be updated with latest attributes
            virtual Common::ErrorCode IsClusterHealthy(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                ClusterUpgradeStateSnapshot const & baseline,
                __inout bool & result,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<std::wstring> & applicationsWithoutAppType) const = 0;
        };
    }
}
