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
    /// Represents the upgrade status of the application instance.
    /// </para>
    /// </summary>
    /// <seealso>
    /// <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-application-upgrade">Service Fabric application upgrade</see>.
    /// </seealso>
    public sealed class ApplicationUpgradeProgress
    {
        #region Fields

        private NativeClient.IFabricApplicationUpgradeProgressResult2 nativeProgress;

        #endregion

        #region Constructors & Initialization

        internal ApplicationUpgradeProgress(NativeClient.IFabricApplicationUpgradeProgressResult2 nativeProgress)
        {
            this.nativeProgress = nativeProgress;
        }

        internal ApplicationUpgradeProgress()
        {
        }

        #endregion

        #region Properties

        /// <summary>
        /// <para>Gets the parameters that describe the behavior of the current upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The parameters that describe the behavior of the current upgrade.</para>
        /// </value>
        public ApplicationUpgradeDescription UpgradeDescription { get; internal set; }

        /// <summary>
        /// <para>Gets the name of the application to be upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application to be upgraded.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// <para>Gets the type name of the application to be upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The type name of the application to be upgraded.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeName)]
        public string ApplicationTypeName { get; internal set; }

        /// <summary>
        /// <para>Gets the version of the application type being upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the application type being upgraded.</para>
        /// </value>
        public string TargetApplicationTypeVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the overall state of the application upgrade process.</para>
        /// </summary>
        /// <value>
        /// <para>The overall state of the application upgrade process.</para>
        /// </value>
        public ApplicationUpgradeState UpgradeState { get; internal set; }

        /// <summary>
        /// <para>Gets the collection of upgrade domains and their upgrade status for this application upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The collection of upgrade domains and their upgrade status for this application upgrade.</para>
        /// </value>
        public ReadOnlyCollection<UpgradeDomainStatus> UpgradeDomains { get; internal set; }

        /// <summary>
        /// <para>Gets the rolling upgrade mode for this upgrade progress.</para>
        /// </summary>
        /// <value>
        /// <para>The rolling upgrade mode for this upgrade progress.</para>
        /// </value>
        public RollingUpgradeMode RollingUpgradeMode { get; internal set; }

        /// <summary>
        /// <para>Gets the next upgrade domain for this upgrade progress.</para>
        /// </summary>
        /// <value>
        /// <para>The next upgrade domain for this upgrade progress.</para>
        /// </value>
        public string NextUpgradeDomain { get; internal set; }

        /// <summary>
        /// <para>Gets the estimated elapsed time spent processing the current overall upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The estimated elapsed time spent processing the current overall upgrade.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.UpgradeDurationInMilliseconds)]
        public TimeSpan UpgradeDuration { get; internal set; }

        /// <summary>
        /// <para>Gets the estimated elapsed time spent processing the current Upgrade Domain.</para>
        /// </summary>
        /// <value>
        /// <para>The estimated elapsed time spent processing the current Upgrade Domain.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.UpgradeDomainDurationInMilliseconds)]
        public TimeSpan CurrentUpgradeDomainDuration { get; internal set; }

        /// <summary>
        /// <para>Gets the evaluations which describe the data and the algorithm 
        /// used by health manager to evaluate the application health. </para>
        /// </summary>
        /// <value>
        /// <para>The evaluations which describe the data and the algorithm used 
        /// by health manager to evaluate the application health.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public IList<HealthEvaluation> UnhealthyEvaluations { get; internal set; }

        /// <summary>
        /// <para>Gives the detailed upgrade progress for nodes in the current upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.UpgradeDomainProgress" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public UpgradeDomainProgress CurrentUpgradeDomainProgress { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the time at which the upgrade started.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The time at which the upgrade started in UTC.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public DateTime? StartTimestampUtc { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the time at which the upgrade failed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The time at which the upgrade failed in UTC.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public DateTime? FailureTimestampUtc { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the category of upgrade failure if the upgrade failed.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The category of upgrade failure. <see cref="System.Fabric.UpgradeFailureReason" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public UpgradeFailureReason? FailureReason { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets structured information about what actions were being performed by the system at the moment of upgrade failure.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The upgrade domain progress details. <see cref="System.Fabric.UpgradeDomainProgress" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public UpgradeDomainProgress UpgradeDomainProgressAtFailure { get; internal set; }


        /// <summary>
        /// <para>Gets the status details of upgrade including failure message. </para>
        /// </summary>
        /// <value>
        /// <para>The status details of upgrade including failure message.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public string UpgradeStatusDetails { get; internal set; }

        /// <summary>
        /// Gets a string representation of the application upgrade progress.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.ApplicationUpgradeProgress"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.CurrentCulture, "[{0}] {1}; ", this.ApplicationTypeName, this.ApplicationName);
            sb.AppendFormat(CultureInfo.CurrentCulture, "TargetApplicationTypeVersion={0}; ", this.TargetApplicationTypeVersion);
            sb.AppendFormat(CultureInfo.CurrentCulture, "RollingUpgradeMode={0}; ", this.RollingUpgradeMode);
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

        internal NativeClient.IFabricApplicationUpgradeProgressResult2 InnerProgress
        {
            get
            {
                return this.nativeProgress;
            }
        }

        #endregion

        #region API

        /// <summary>
        /// <para>Specifies a helper method that returns a collection of upgrade domains that have changed to the <see cref="System.Fabric.ApplicationUpgradeState.RollingForwardCompleted" /> state or the <see cref="System.Fabric.ApplicationUpgradeState.RollingForwardInProgress" /> state since the <see cref="System.Fabric.ApplicationUpgradeProgress.GetChangedUpgradeDomains(System.Fabric.ApplicationUpgradeProgress)" /> method was called.</para>
        /// </summary>
        /// <param name="previousProgress">
        /// <para>The previous <see cref="System.Fabric.ApplicationUpgradeProgress" /> object.</para>
        /// </param>
        /// <returns>
        /// <para>A helper method that returns a collection of upgrade domains.</para>
        /// </returns>
        /// <exception cref="System.ArgumentException">
        /// <para>The application name or the application type of the <paramref name="previousProgress" /> parameter does not match the application name or application type of this object.</para>
        /// </exception>
        public ReadOnlyCollection<UpgradeDomainStatus> GetChangedUpgradeDomains(ApplicationUpgradeProgress previousProgress)
        {
            return InteropHelpers.ApplicationUpgradeProgressHelper.GetChangedUpgradeDomainsFromNative(
                (previousProgress == null) ? null : previousProgress.InnerProgress,
                this.nativeProgress);
        }

        #endregion
    }
}