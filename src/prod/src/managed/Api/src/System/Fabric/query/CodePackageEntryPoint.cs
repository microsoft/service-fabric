// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;
    using System.Fabric.Common.Serialization;

    /// <summary>
    /// <para>Represents a code package entry point.</para>
    /// </summary>
    public sealed class CodePackageEntryPoint
    {
        internal CodePackageEntryPoint(
            string entryPointLocation,
            long processId,
            string runAsUserName,
            EntryPointStatus entrypointStatus,
            DateTime nextActivationUtc,
            CodePackageEntryPointStatistics statistics,
            Int64 codePackageInstanceId)
        {
            this.EntryPointLocation = entryPointLocation;
            this.ProcessId = processId;
            this.RunAsUserName = runAsUserName;
            this.EntryPointStatus = entrypointStatus;
            this.NextActivationUtc = nextActivationUtc;
            this.Statistics = statistics;
            this.CodePackageInstanceId = codePackageInstanceId;
        }

        private CodePackageEntryPoint() { }

        /// <summary>
        /// <para>Gets the entry point location.</para>
        /// </summary>
        /// <value>
        /// <para>The entry point location.</para>
        /// </value>
        public string EntryPointLocation { get; private set; }

        /// <summary>
        /// <para>Gets the process ID.</para>
        /// </summary>
        /// <value>
        /// <para>The process ID.</para>
        /// </value>
        public long ProcessId { get; private set; }

        /// <summary>
        /// <para>Gets the run as user name.</para>
        /// </summary>
        /// <value>
        /// <para>The run as user name.</para>
        /// </value>
        public string RunAsUserName { get; private set; }

        /// <summary>
        /// <para>Gets the statistics.</para>
        /// </summary>
        /// <value>
        /// <para>The statistics.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.CodePackageEntryPointStatistics)]
        public CodePackageEntryPointStatistics Statistics { get; private set; }

        /// <summary>
        /// <para>Gets the entry point status.</para>
        /// </summary>
        /// <value>
        /// <para>The entry point status.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public EntryPointStatus EntryPointStatus { get; private set; }

        /// <summary>
        /// <para>Gets the next activation UTC.</para>
        /// </summary>
        /// <value>
        /// <para>The next activation UTC.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NextActivationTime)]
        public DateTime NextActivationUtc { get; private set; }

        /// <summary>
        /// <para>
        /// Gets the code package instance id.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The code package instance id.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.InstanceId)]
        public Int64 CodePackageInstanceId { get; private set; }

        internal static unsafe CodePackageEntryPoint FromNative(NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT nativeEntryPoint)
        {
            NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS* nativeEntryPointStatistics = (NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_STATISTICS*)nativeEntryPoint.Statistics;

            Int64 instanceId = 0;
            if (nativeEntryPoint.Reserved != IntPtr.Zero)
            {
                NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_EX1* nativeResultItemEx1 = (NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_EX1*)nativeEntryPoint.Reserved;
                instanceId = nativeResultItemEx1->CodePackageInstanceId;
            }

            return new CodePackageEntryPoint(
                NativeTypes.FromNativeString(nativeEntryPoint.EntryPointLocation),
                nativeEntryPoint.ProcessId,
                NativeTypes.FromNativeString(nativeEntryPoint.RunAsUserName),
                (EntryPointStatus)nativeEntryPoint.EntryPointStatus,
                NativeTypes.FromNativeFILETIME(nativeEntryPoint.NextActivationUtc),
                CodePackageEntryPointStatistics.FromNative(*nativeEntryPointStatistics),
                instanceId);
        }
    }
}