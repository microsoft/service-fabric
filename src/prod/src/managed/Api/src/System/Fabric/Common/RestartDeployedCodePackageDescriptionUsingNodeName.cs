// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;

    internal sealed class RestartDeployedCodePackageDescriptionUsingNodeName
    {
        public RestartDeployedCodePackageDescriptionUsingNodeName(
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId)
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId?? string.Empty;
            this.CodePackageName = codePackageName;
            this.CodePackageInstanceId = codePackageInstanceId;
        }

        public string NodeName
        {
            get;
            internal set;
        }

        public Uri ApplicationName
        {
            get;
            internal set;
        }

        public string ServiceManifestName
        {
            get;
            internal set;
        }

        public string CodePackageName
        {
            get;
            internal set;
        }

        public long CodePackageInstanceId
        {
            get;
            internal set;
        }

        public string ServicePackageActivationId
        {
            get;
            internal set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "NodeName: {0}, ApplicationName: {1}, ServiceManifestName: {2}, ServicePackageActivationId: {3}, CodePackageName: {4}, CodePackageInstanceId: {5}.",
                this.NodeName,
                this.ApplicationName,
                this.ServiceManifestName,
                this.ServicePackageActivationId,
                this.CodePackageName,
                this.CodePackageInstanceId);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeRestartDeployedCodePackageDescriptionUsingNodeName = new NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME();

            nativeRestartDeployedCodePackageDescriptionUsingNodeName.NodeName = pinCollection.AddObject(this.NodeName);

            nativeRestartDeployedCodePackageDescriptionUsingNodeName.ApplicationName = pinCollection.AddObject(this.ApplicationName);

            nativeRestartDeployedCodePackageDescriptionUsingNodeName.ServiceManifestName = pinCollection.AddObject(this.ServiceManifestName);

            nativeRestartDeployedCodePackageDescriptionUsingNodeName.CodePackageName = pinCollection.AddObject(this.CodePackageName);

            nativeRestartDeployedCodePackageDescriptionUsingNodeName.CodePackageInstanceId = this.CodePackageInstanceId;

            if (!string.IsNullOrWhiteSpace(this.ServicePackageActivationId))
            {
                var ex1 = new NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME_EX1();
                ex1.ServicePackageActivationId = pinCollection.AddObject(this.ServicePackageActivationId);

                nativeRestartDeployedCodePackageDescriptionUsingNodeName.Reserved = pinCollection.AddBlittable(ex1);
            }
            
            return pinCollection.AddBlittable(nativeRestartDeployedCodePackageDescriptionUsingNodeName);
        }

        internal static unsafe RestartDeployedCodePackageDescriptionUsingNodeName CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME native = *(NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME*)nativeRaw;

            string nodeName = NativeTypes.FromNativeString(native.NodeName);

            Uri applicationName = NativeTypes.FromNativeUri(native.ApplicationName);

            string serviceManifestName = NativeTypes.FromNativeString(native.ServiceManifestName);

            string codePackageName = NativeTypes.FromNativeString(native.CodePackageName);

            long codePackageInstanceId = native.CodePackageInstanceId;

            var servicePackageActivationId = string.Empty;
            if (native.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME_EX1*)native.Reserved;
                servicePackageActivationId = NativeTypes.FromNativeString(ex1->ServicePackageActivationId);
            }

            return new RestartDeployedCodePackageDescriptionUsingNodeName(
                nodeName, 
                applicationName, 
                serviceManifestName,
                servicePackageActivationId,
                codePackageName, 
                codePackageInstanceId);
        }
    }
}
