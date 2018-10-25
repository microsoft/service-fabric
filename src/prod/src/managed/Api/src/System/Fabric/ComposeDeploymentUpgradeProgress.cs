// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using Common.Serialization;
    using Globalization;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using Text;

    /// <summary>
    /// <para>
    /// Represents the upgrade progress of the compose deployment.
    /// </para>
    /// </summary>
    public sealed class ComposeDeploymentUpgradeProgress
    {
        #region Constructors & Initialization

        internal ComposeDeploymentUpgradeProgress()
        {
        }

        #endregion

        #region Properties

        /// <summary>
        /// <para>Gets the name of the compose deployment to be upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the compose deployment to be upgraded.</para>
        /// </value>
        public string DeploymentName { get; internal set; }

        /// <summary>
        /// <para>Gets the name of the application to be upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application to be upgraded.</para>
        /// </value>
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the description of the policy used for upgrading this compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The description of the policy used for upgrading this compose deployment.</para>
        /// </value>
        public UpgradePolicyDescription UpgradePolicyDescription { get; set; }

        /// <summary>
        /// <para>Gets the version of the application type being upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the application type being upgraded.</para>
        /// </value>
        public string TargetApplicationTypeVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the overall state of the compose deployment upgrade process.</para>
        /// </summary>
        /// <value>
        /// <para>The overall state of the compose deployment upgrade process.</para>
        /// </value>
        public ComposeDeploymentUpgradeState UpgradeState { get; internal set; }

        /// <summary>
        /// <para>Gets the next upgrade domain for this upgrade progress.</para>
        /// </summary>
        /// <value>
        /// <para>The next upgrade domain for this upgrade progress.</para>
        /// </value>
        public string NextUpgradeDomain { get; internal set; }

        /// <summary>
        /// <para>Gets the collection of upgrade domains and their upgrade status for this compose deployment upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of upgrade domains and their upgrade status for this compose deployment  upgrade.</para>
        /// </value>
        public ReadOnlyCollection<UpgradeDomainStatus> UpgradeDomains { get; internal set; }


        /// <summary>
        /// <para>Gets the estimated elapsed time spent processing the current overall upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The estimated elapsed time spent processing the current overall upgrade.</para>
        /// </value>
        public TimeSpan UpgradeDuration { get; internal set; }

        /// <summary>
        /// <para>Gets the estimated elapsed time spent processing the current Upgrade Domain.</para>
        /// </summary>
        /// <value>
        /// <para>The estimated elapsed time spent processing the current Upgrade Domain.</para>
        /// </value>
        public TimeSpan CurrentUpgradeDomainDuration { get; internal set; }

        /// <summary>
        /// <para>Gets the evaluations which describe the data and the algorithm 
        /// used by health manager to evaluate the health of the application that this compose deployment is upgrading. </para>
        /// </summary>
        /// <value>
        /// <para>The evaluations which describe the data and the algorithm
        /// used by health manager to evaluate the health of the application that this compose deployment is upgrading.</para>
        /// </value>
        public IList<HealthEvaluation> ApplicationUnhealthyEvaluations { get; internal set; }

        /// <summary>
        /// <para>Gives the detailed upgrade progress for nodes in the current upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.UpgradeDomainProgress" />.</para>
        /// </value>
        public UpgradeDomainProgress CurrentUpgradeDomainProgress { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the time at which the upgrade started.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The time at which the upgrade started in UTC.</para>
        /// </value>
        public DateTime? StartTimestampUtc { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the time at which the upgrade failed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The time at which the upgrade failed in UTC.</para>
        /// </value>
        public DateTime? FailureTimestampUtc { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the category of upgrade failure if the upgrade failed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The category of upgrade failure. <see cref="System.Fabric.UpgradeFailureReason" />.</para>
        /// </value>
        public UpgradeFailureReason? FailureReason { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets structured information about what actions were being performed by the system at the moment of upgrade failure.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The upgrade domain progress details. <see cref="System.Fabric.UpgradeDomainProgress" />.</para>
        /// </value>
        public UpgradeDomainProgress UpgradeDomainProgressAtFailure { get; internal set; }


        /// <summary>
        /// <para>Gets the status details of compose deployment upgrade including failure message. </para>
        /// </summary>
        /// <value>
        /// <para>The status details of compose deployment upgrade including failure message.</para>
        /// </value>
        public string UpgradeStatusDetails { get; internal set; }

        /// <summary>
        /// <para>Gets the status details of application upgrade including failure message. </para>
        /// </summary>
        /// <value>
        /// <para>The status details of application upgrade including failure message.</para>
        /// </value>
        public string ApplicationUpgradeStatusDetails { get; internal set; }

        /// <summary>
        /// Gets a string representation of the compose deployment upgrade progress.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.ComposeDeploymentUpgradeProgress"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.CurrentCulture, "DeploymentName={0}; ", this.DeploymentName);
            sb.AppendFormat(CultureInfo.CurrentCulture, "ApplicationName={0}; ", this.ApplicationName);
            sb.AppendFormat(CultureInfo.CurrentCulture, "TargetApplicationTypeVersion={0}; ", this.TargetApplicationTypeVersion);
            sb.AppendFormat(CultureInfo.CurrentCulture, "UpgradeState={0}; ", this.UpgradeState);
            sb.AppendFormat(CultureInfo.CurrentCulture, "UpgradeStatusDetails={0}; ", this.UpgradeStatusDetails);
            sb.AppendLine();

            if (this.UpgradeDomains.Count > 0)
            {
                foreach (UpgradeDomainStatus domainStatus in this.UpgradeDomains)
                {
                    sb.AppendLine(domainStatus.ToString());
                }
            }
            else
            {
                sb.AppendLine("UpgradeDomains: <empty>");
            }

            return sb.ToString();
        }

        internal static unsafe ComposeDeploymentUpgradeProgress CreateFromNative(
            NativeClient.IFabricComposeDeploymentUpgradeProgressResult nativeResult)
        {
            ComposeDeploymentUpgradeProgress result = new ComposeDeploymentUpgradeProgress();
            IntPtr upgradeProgressPtr = nativeResult.get_UpgradeProgress();
            if (upgradeProgressPtr != IntPtr.Zero)
            {
                var castedUpgradeProgressPtr = (NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS*)upgradeProgressPtr;

                result.ApplicationName = NativeTypes.FromNativeUri(castedUpgradeProgressPtr->ApplicationName);
                result.DeploymentName = NativeTypes.FromNativeString(castedUpgradeProgressPtr->DeploymentName);
                
                // Upgrade policy
                if (castedUpgradeProgressPtr->UpgradePolicyDescription != IntPtr.Zero)
                {
                    RollingUpgradePolicyDescription policy = null;

                    var castedPolicyPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)castedUpgradeProgressPtr->UpgradePolicyDescription;

                    if (castedPolicyPtr->RollingUpgradeMode == NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED)
                    {
                        policy = MonitoredRollingApplicationUpgradePolicyDescription.FromNative(castedUpgradeProgressPtr->UpgradePolicyDescription);
                    }
                    else
                    {
                        policy = RollingUpgradePolicyDescription.FromNative(castedUpgradeProgressPtr->UpgradePolicyDescription);
                    }

                    result.UpgradePolicyDescription = policy;
                }

                result.TargetApplicationTypeVersion = NativeTypes.FromNativeString(castedUpgradeProgressPtr->TargetApplicationTypeVersion);

                switch (castedUpgradeProgressPtr->UpgradeState)
                {
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_PROVISIONING_TARGET:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_PENDING:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_CURRENT:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_TARGET:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_COMPLETED:
                    case NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_FAILED:
                        result.UpgradeState = (ComposeDeploymentUpgradeState)castedUpgradeProgressPtr->UpgradeState;
                        break;

                    default:
                        result.UpgradeState = ComposeDeploymentUpgradeState.Invalid;
                        break;

                }

                result.NextUpgradeDomain = NativeTypes.FromNativeString(castedUpgradeProgressPtr->NextUpgradeDomain);
                if (castedUpgradeProgressPtr->UpgradeDomains != IntPtr.Zero)
                {
                    var upgradeDomainsPtr = (NativeTypes.FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION_LIST*)castedUpgradeProgressPtr->UpgradeDomains;
                    uint count = upgradeDomainsPtr->Count;
                    result.UpgradeDomains = Interop.InteropHelpers.UpgradeDomainHelper.CreateUpgradeDomainsFromNative(count, upgradeDomainsPtr->Items);
                }

                result.UpgradeDuration = TimeSpan.FromSeconds(castedUpgradeProgressPtr->UpgradeDurationInSeconds);
                result.CurrentUpgradeDomainDuration = TimeSpan.FromSeconds(castedUpgradeProgressPtr->CurrentUpgradeDomainDurationInSeconds);
                result.ApplicationUnhealthyEvaluations = HealthEvaluation.FromNativeList(castedUpgradeProgressPtr->ApplicationUnhealthyEvaluations);
                result.CurrentUpgradeDomainProgress = UpgradeDomainProgress.FromNative(castedUpgradeProgressPtr->CurrentUpgradeDomainProgress);
                result.StartTimestampUtc = NativeTypes.FromNullableNativeFILETIME(castedUpgradeProgressPtr->StartTimestampUtc);
                result.FailureTimestampUtc = NativeTypes.FromNullableNativeFILETIME(castedUpgradeProgressPtr->FailureTimestampUtc);

                if (result.FailureTimestampUtc.HasValue)
                {
                    switch (castedUpgradeProgressPtr->FailureReason)
                    {
                        case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED:
                        case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK:
                        case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT:
                        case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT:
                        case NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE:
                            result.FailureReason = (UpgradeFailureReason)castedUpgradeProgressPtr->FailureReason;
                            break;

                        default:
                            result.FailureReason = UpgradeFailureReason.None;
                            break;
                    }

                    result.UpgradeDomainProgressAtFailure = UpgradeDomainProgress.FromNative(castedUpgradeProgressPtr->UpgradeDomainProgressAtFailure);
                }

                result.ApplicationUpgradeStatusDetails = NativeTypes.FromNativeString(castedUpgradeProgressPtr->ApplicationUpgradeStatusDetails);
                result.UpgradeStatusDetails = NativeTypes.FromNativeString(castedUpgradeProgressPtr->UpgradeStatusDetails);
            }

            return result;
        }

        #endregion
    }
}