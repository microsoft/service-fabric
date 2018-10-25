// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a code package entry point statistic.</para>
    /// </summary>
    public sealed class CodePackageEntryPointStatistics
    {
        internal CodePackageEntryPointStatistics(
             long lastExitCode,
             DateTime lastActivationUtc,
             DateTime lastExitUtc,
             DateTime lastSuccessfulActivationUtc,
             DateTime lastSuccessfulExitUtc,
             long activationCount,
             long activationFailureCount,
             long continuousActivationFailureCount,
             long exitCount,
             long exitFailureCount,
             long continuousExitFailureCount)
        {
            this.LastExitCode = lastExitCode;
            this.LastActivationUtc = lastActivationUtc;
            this.LastExitUtc = lastExitUtc;
            this.LastSuccessfulActivationUtc = lastSuccessfulActivationUtc;
            this.LastSuccessfulExitUtc = lastSuccessfulExitUtc;
            this.ActivationCount = activationCount;
            this.ActivationFailureCount = activationFailureCount;
            this.ContinuousActivationFailureCount = continuousActivationFailureCount;
            this.ExitCount = exitCount;
            this.ExitFailureCount = exitFailureCount;
            this.ContinuousExitFailureCount = continuousExitFailureCount;
        }

        private CodePackageEntryPointStatistics() { }

        /// <summary>
        /// <para>Gets the exit code.</para>
        /// </summary>
        /// <value>
        /// <para>The last exit code.</para>
        /// </value>
        public long LastExitCode { get; private set; }

        /// <summary>
        /// <para>Gets the last activation time in UTC.</para>
        /// </summary>
        /// <value>
        /// <para>The last activation time in UTC.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.LastActivationTime)]
        public DateTime LastActivationUtc { get; private set; }

        /// <summary>
        /// <para>Gets the last exit time in UTC.</para>
        /// </summary>
        /// <value>
        /// <para>The last exit time in UTC.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.LastExitTime)]
        public DateTime LastExitUtc { get; private set; }

        /// <summary>
        /// <para>Gets the last successful activation time in UTC.</para>
        /// </summary>
        /// <value>
        /// <para>The last successful activation time in UTC.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.LastSuccessfulActivationTime)]
        public DateTime LastSuccessfulActivationUtc { get; private set; }

        /// <summary>
        /// <para>Gets the last successful exit time in UTC.</para>
        /// </summary>
        /// <value>
        /// <para>The last successful exit time in UTC.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.LastSuccessfulExitTime)]
        public DateTime LastSuccessfulExitUtc { get; private set; }

        /// <summary>
        /// <para>Gets the activation count.</para>
        /// </summary>
        /// <value>
        /// <para>The activation count.</para>
        /// </value>
        public long ActivationCount { get; private set; }

        /// <summary>
        /// <para>Gets the activation failure count.</para>
        /// </summary>
        /// <value>
        /// <para>The activation failure count.</para>
        /// </value>
        public long ActivationFailureCount { get; private set; }

        /// <summary>
        /// <para>Gets the continuous activation failure count.</para>
        /// </summary>
        /// <value>
        /// <para>The continuous activation failure count.</para>
        /// </value>
        public long ContinuousActivationFailureCount { get; private set; }

        /// <summary>
        /// <para>Gets the exit count.</para>
        /// </summary>
        /// <value>
        /// <para>The exit count.</para>
        /// </value>
        public long ExitCount { get; private set; }

        /// <summary>
        /// <para>Gets the exit failure count.</para>
        /// </summary>
        /// <value>
        /// <para>The exit failure count.</para>
        /// </value>
        public long ExitFailureCount { get; private set; }

        /// <summary>
        /// <para>Gets the continuous exit failure count.</para>
        /// </summary>
        /// <value>
        /// <para>The continuous exit failure count.</para>
        /// </value>
        public long ContinuousExitFailureCount { get; private set; }

        internal static unsafe CodePackageEntryPointStatistics FromNative(NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS nativeEntryPointStatistics)
        {
            return new CodePackageEntryPointStatistics(
                nativeEntryPointStatistics.LastExitCode,
                NativeTypes.FromNativeFILETIME(nativeEntryPointStatistics.LastActivationUtc),
                NativeTypes.FromNativeFILETIME(nativeEntryPointStatistics.LastExitUtc),
                NativeTypes.FromNativeFILETIME(nativeEntryPointStatistics.LastSuccessfulActivationUtc),
                NativeTypes.FromNativeFILETIME(nativeEntryPointStatistics.LastSuccessfulExitUtc),
                nativeEntryPointStatistics.ActivationCount,
                nativeEntryPointStatistics.ActivationFailureCount,
                nativeEntryPointStatistics.ContinuousActivationFailureCount,
                nativeEntryPointStatistics.ExitCount,
                nativeEntryPointStatistics.ExitFailureCount,
                nativeEntryPointStatistics.ContinuousExitFailureCount);
        }

    }
}