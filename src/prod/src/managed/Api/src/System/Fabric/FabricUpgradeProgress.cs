// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using Globalization;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using Text;
    /// <summary>
    /// <para>Encapsulates the progress of a Service Fabric upgrade.</para>
    /// </summary>
    public sealed class FabricUpgradeProgress
    {
        #region Fields

        private NativeClient.IFabricUpgradeProgressResult3 nativeProgress;

        #endregion

        #region Constructors & Initialization

        internal FabricUpgradeProgress(NativeClient.IFabricUpgradeProgressResult3 nativeProgress)
        {
            this.nativeProgress = nativeProgress;
        }

        internal FabricUpgradeProgress()
        {
        }

        #endregion

        #region Properties

        /// <summary>
        /// <para>Gets the upgrade parameter details that describe the behavior of the current upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade parameter details that describe the behavior of the current upgrade.</para>
        /// </value>
        public FabricUpgradeDescription UpgradeDescription { get; internal set; }

        /// <summary>
        /// <para>Gets the target code version for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The target code version for this  upgrade.</para>
        /// </value>
        public string TargetCodeVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the target configuration version for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The target configuration version for this  upgrade.</para>
        /// </value>
        public string TargetConfigVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the upgrade state for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade state for this  upgrade.</para>
        /// </value>
        public FabricUpgradeState UpgradeState { get; internal set; }

        /// <summary>
        /// <para>Gets the list of upgrade domains.</para>
        /// </summary>
        /// <value>
        /// <para>The list of upgrade domains.</para>
        /// </value>
        public ReadOnlyCollection<UpgradeDomainStatus> UpgradeDomains { get; internal set; }

        /// <summary>
        /// <para>Gets the rolling upgrade mode for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The rolling upgrade mode for this  upgrade.</para>
        /// </value>
        public RollingUpgradeMode RollingUpgradeMode { get; internal set; }

        /// <summary>
        /// <para>Gets the next upgrade domain for this  upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The next upgrade domain for this  upgrade.</para>
        /// </value>
        public string NextUpgradeDomain { get; internal set; }

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
        /// <para>Gets the health evaluations which describe the data and the algorithm used by health manager to evaluate cluster health. Only populated when the cluster’s aggregated health state is  <see cref="System.Fabric.Health.HealthState.Error" />. When the upgrade rolls back because of health evaluation, it provides a snapshot view of the health at the time the health check was performed.</para>
        /// </summary>
        /// <value>
        /// <para>The unhealthy evaluations that led to current aggregated health state. The items in the list can be of the following types: <see cref="System.Fabric.Health.ApplicationsHealthEvaluation" />, <see cref="System.Fabric.Health.NodesHealthEvaluation" />, <see cref="System.Fabric.Health.UpgradeDomainNodesHealthEvaluation" />, <see cref="System.Fabric.Health.SystemApplicationHealthEvaluation" /> oror <see cref="System.Fabric.Health.EventHealthEvaluation" />.</para>
        /// </value>
        public IList<HealthEvaluation> UnhealthyEvaluations { get; internal set; }

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
        /// <para>The upgrade progress details. <see cref="System.Fabric.UpgradeDomainProgress" />.</para>
        /// </value>
        public UpgradeDomainProgress UpgradeDomainProgressAtFailure { get; internal set; }

        /// <summary>
        /// Gets a string representation of the fabric upgrade progress.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.FabricUpgradeProgress"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.CurrentCulture, "TargetCodeVersion={0}; ", this.TargetCodeVersion);
            sb.AppendFormat(CultureInfo.CurrentCulture, "TargetConfigVersion={0}; ", this.TargetConfigVersion);
            sb.AppendFormat(CultureInfo.CurrentCulture, "UpgradeState={0}; ", this.UpgradeState);
            sb.AppendFormat(CultureInfo.CurrentCulture, "UpgradeMode={0}; ", this.RollingUpgradeMode);

            if (this.FailureReason != null)
            {
                sb.AppendFormat(CultureInfo.CurrentCulture, "FailureReason={0}; ", this.FailureReason);
            }

            if (this.UpgradeDomains.Count > 0)
            {
                sb.AppendLine(string.Format(CultureInfo.CurrentCulture, "UpgradeDomains: "));
                foreach (var domainStatus in this.UpgradeDomains)
                {
                    sb.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}", domainStatus));
                }
            }

            if (this.UnhealthyEvaluations != null && this.UnhealthyEvaluations.Count > 0)
            {
                sb.AppendLine(string.Format(CultureInfo.CurrentCulture, "UnhealthyEvaluations: "));
                foreach (var unhealthyEvaluation in this.UnhealthyEvaluations)
                {
                    sb.AppendLine(string.Format(CultureInfo.CurrentCulture, "{0}", unhealthyEvaluation));
                }
            }

            return sb.ToString();
        }

        internal NativeClient.IFabricUpgradeProgressResult3 InnerProgress
        {
            get
            {
                return this.nativeProgress;
            }
        }

        #endregion

        #region API

        /// <summary>
        /// <para>Gets the list of changed upgrade domains.</para>
        /// </summary>
        /// <param name="previousProgress">
        /// <para>The previous progress report from this  upgrade process.</para>
        /// </param>
        /// <returns>
        /// <para>The list of changed upgrade domains.</para>
        /// </returns>
        public ReadOnlyCollection<UpgradeDomainStatus> GetChangedUpgradeDomains(FabricUpgradeProgress previousProgress)
        {
            return InteropHelpers.FabricUpgradeProgressHelper.GetChangedUpgradeDomainsFromNative(
                (previousProgress == null) ? null : previousProgress.InnerProgress,
                this.nativeProgress);
        }

        #endregion
    }
}