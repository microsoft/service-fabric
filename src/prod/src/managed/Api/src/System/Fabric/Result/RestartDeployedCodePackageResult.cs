// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// Returns Restart deployed code package result object. 
    /// </summary>
    /// <remarks>
    /// This class returns nodeName, applicationName, serviceManifestName, codePackageName, codePackageInstanceId and SelectedReplica information 
    /// for which the deployed code package action was called. ReplicaSelector will be none in case application is selecetd using nodename, 
    /// application name, service manifest, code package name, and code package instance id.
    /// </remarks>
    [Serializable]
    public class RestartDeployedCodePackageResult
    {
        /// <summary>
        /// Restart deployed code package result constructor.
        /// </summary>
        /// <param name="nodeName">node name</param>
        /// <param name="applicationName">application name</param>
        /// <param name="serviceManifestName">service manifest name</param>
        /// <param name="servicePackageActivationId">service package ActivationId</param>
        /// <param name="codePackageName">code package name</param>
        /// <param name="codePackageInstanceId">code package instance id</param>
        /// <param name="selectedReplica">selected replica</param>
        internal RestartDeployedCodePackageResult(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            SelectedReplica selectedReplica)
        {
            ReleaseAssert.AssertIfNull(applicationName, "Application name cannot be null");
            this.ApplicationName = applicationName;
            ReleaseAssert.AssertIf(string.IsNullOrEmpty(nodeName), "Node name cannot be null or empty");
            this.NodeName = nodeName;
            ReleaseAssert.AssertIf(string.IsNullOrEmpty(serviceManifestName), "Service manifestName cannot be null or empty");
            this.ServiceManifestName = serviceManifestName;
            ReleaseAssert.AssertIf(string.IsNullOrEmpty(codePackageName), "Code package name cannot be null or empty");
            this.CodePackageName = codePackageName;
            ReleaseAssert.AssertIf(codePackageInstanceId == 0, "Code package instance id cannot be zero");
            this.CodePackageInstanceId = codePackageInstanceId;
            this.SelectedReplica = selectedReplica;

            ReleaseAssert.AssertIfNull(servicePackageActivationId, "ServicePackageActivationId cannot be null");
            this.ServicePackageActivationId = servicePackageActivationId;
        }

        /// <summary>
        /// Gets Application name.
        /// </summary>
        /// <value>The application name.</value>
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// Gets node name.
        /// </summary>
        /// <value>The node name.</value>
        public string NodeName { get; private set; }

        /// <summary>
        /// Gets service manifest name.
        /// </summary>
        /// <value>The service manifest name.</value>
        public string ServiceManifestName { get; private set; }

        /// <summary>
        /// Gets the ActivationId of service package.
        /// </summary>
        /// <value>
        /// <para>
        /// A string representing ActivationId of deployed service package. 
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specfied, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Result.RestartDeployedCodePackageResult.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId { get; private set; }

        /// <summary>
        /// Gets code package name.
        /// </summary>
        /// <value>The code package name.</value>
        public string CodePackageName { get; private set; }

        /// <summary>
        /// Gets code package instance id.
        /// </summary>
        /// <value>The code package instance id, as a long.</value>
        public long CodePackageInstanceId { get; private set; }

        /// <summary>
        /// Gets selected replica.
        /// </summary>
        /// <value>The SelectedReplica object.</value>
        public SelectedReplica SelectedReplica { get; private set; }

        /// <summary>
        /// Formats the data inside RestartDeployedCodePackageResult as a string.
        /// </summary>
        /// <returns>The formatted string.</returns>
        public override string ToString()
        {
            return string.Format("NodeName: {0}, ApplicationName: {1}, ServiceManifestName: {2}, ServicePackageActivationId: {3}, CodePackageName: {4}, CodePackageInstanceId: {5}, SelectedReplica: {6}",
                this.NodeName, 
                this.ApplicationName, 
                this.ServiceManifestName,
                this.ServicePackageActivationId,
                this.CodePackageName, 
                this.CodePackageInstanceId, 
                this.SelectedReplica);
        }

        internal unsafe static RestartDeployedCodePackageResult CreateFromNative(IntPtr nativeResult)
        {
            NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_RESULT publicResult = *(NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_RESULT*)nativeResult;

            string nodeName = NativeTypes.FromNativeString(publicResult.NodeName);

            Uri applicationName = NativeTypes.FromNativeUri(publicResult.ApplicationName);

            string serviceManifestName = NativeTypes.FromNativeString(publicResult.ServiceManifestName);

            string codePackageName = NativeTypes.FromNativeString(publicResult.CodePackageName);

            long codePackageInstanceId = publicResult.CodePackageInstanceId;

            var servicePackageActivationId = string.Empty;
            if (publicResult.Reserved != IntPtr.Zero)
            {
                var nativeResultEx1 = (NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_RESULT_EX1*)publicResult.Reserved;
                servicePackageActivationId = NativeTypes.FromNativeString(nativeResultEx1->ServicePackageActivationId);
            }

            return new RestartDeployedCodePackageResult(
                nodeName, 
                applicationName, 
                serviceManifestName,
                servicePackageActivationId,
                codePackageName, 
                codePackageInstanceId, 
                null);
        }
    }
}