// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes an instance of an application’s service host running on a Service Fabric Node.</para>
    /// </summary>
    public sealed class DeployedApplication
    {
        internal DeployedApplication(
            Uri applicationName,
            string applicationTypeName,
            DeploymentStatus deploymentStatus,
            string workDirectory,
            string logDirectory,
            string tempDirectory,
            Health.HealthState healthState)
        {
            this.ApplicationName = applicationName;
            this.ApplicationTypeName = applicationTypeName;
            this.DeployedApplicationStatus = deploymentStatus;
            this.WorkDirectory = workDirectory;
            this.LogDirectory = logDirectory;
            this.TempDirectory = tempDirectory;
            this.HealthState = healthState;
        }

        private DeployedApplication() { }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The application name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// <para>Gets the application type name.</para>
        /// </summary>
        /// <value>
        /// <para>The application type name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.TypeName)]
        public string ApplicationTypeName { get; private set; }

        /// <summary>
        /// <para>Gets the status of the deployed application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the deployed application instance.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public DeploymentStatus DeployedApplicationStatus { get; private set; }

        /// <summary>
        /// <para>Gets the work directory path used by the deployed application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The work directory path used by the deployed application instance.</para>
        /// </value>
        public string WorkDirectory { get; private set; }

        /// <summary>
        /// <para>Gets the log directory path used by the deployed application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The log directory path used by the deployed application instance.</para>
        /// </value>
        public string LogDirectory { get; private set; }

        /// <summary>
        /// <para>Gets the temp directory path used by the deployed application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The temp directory path used by the deployed application instance.</para>
        /// </value>
        public string TempDirectory { get; private set; }

        /// <summary>
        /// <para>Gets the aggregated health state of the deployed application instance.</para>
        /// </summary>
        /// <value>
        ///     <para>The aggregated health state of the deployed application instance.</para>
        /// </value>
        public Health.HealthState HealthState { get; private set; }

        internal static unsafe DeployedApplication CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM nativeResultItem)
        {
            var nativeResultItemEx = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX*)nativeResultItem.Reserved;
            var healthState = Health.HealthState.Unknown;

            if (nativeResultItemEx->Reserved != IntPtr.Zero)
            {
                var nativeResultItemEx2 = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM_EX2*) nativeResultItemEx->Reserved;
                healthState = (Health.HealthState)nativeResultItemEx2->HealthState;
            }

            return new DeployedApplication(
                NativeTypes.FromNativeUri(nativeResultItem.ApplicationName),
                NativeTypes.FromNativeString(nativeResultItem.ApplicationTypeName),
                (DeploymentStatus)nativeResultItem.DeployedApplicationStatus,
                NativeTypes.FromNativeString(nativeResultItemEx->WorkDirectory),
                NativeTypes.FromNativeString(nativeResultItemEx->LogDirectory),
                NativeTypes.FromNativeString(nativeResultItemEx->TempDirectory),
                healthState);
        }
    }
}