// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Describes possible upgrade failure reasons.
    /// </para>
    /// </summary>
    public enum UpgradeFailureReason
    {
        /// <summary>
        /// <para>
        /// There is no upgrade failures.
        /// </para>
        /// </summary>
        None = NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_NONE,

        /// <summary>
        /// <para>
        /// The upgrade was interrupted or manually rolled back.
        /// </para>
        /// </summary>
        Interrupted = NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED,

        /// <summary>
        /// <para>
        /// The upgrade failed because of health checks.
        /// </para>
        /// </summary>
        HealthCheck = NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK,

        /// <summary>
        /// <para>
        /// The upgrade domain timeout expired.
        /// </para>
        /// </summary>
        UpgradeDomainTimeout = NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT,

        /// <summary>
        /// <para>
        /// The overall upgrade timeout expired.
        /// </para>
        /// </summary>
        OverallUpgradeTimeout = NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT,

        /// <summary>
        /// <para>
        /// The upgrade failed because of a processing error.
        /// For example: failure to process default service(s). 
        /// See <see cref="System.Fabric.ApplicationUpgradeProgress.UpgradeStatusDetails "/> for details.
        /// </para>
        /// </summary>
        ProcessingFailure = NativeTypes.FABRIC_UPGRADE_FAILURE_REASON.FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE,
    }
}
