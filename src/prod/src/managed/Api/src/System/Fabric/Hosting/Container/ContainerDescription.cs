// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ContainerDescription
    {
        internal ContainerDescription()
        {
        }

        internal string ApplicationName { get; set; }

        internal string ServiceName { get; set; }

        internal string ApplicationId { get; set; }

        internal string ContainerName { get; set; }

        internal string DeploymentFolder { get; set; }

        internal string NodeWorkFolder { get; set; }

        internal ContainerNetworkConfigDescription ContainerNetworkConfig { get; set; }

        internal string EntryPoint { get; set; }

        internal string HostName { get; set; }

        internal string GroupContainerName { get; set; }

        internal ContainerIsolationMode IsolationMode { get; set; }

        internal IDictionary<string, string> PortBindings { get; set; }

        internal ContainerLogConfigDescription LogConfig { get; set; }

        internal List<ContainerVolumeDescription> Volumes { get; set; }

        internal List<ContainerLabelDescription> Labels { get; set; }

        internal List<string> DnsServers { get; set; }

        internal List<string> SecurityOptions { get; set; }

        internal RepositoryCredentialDescription RepositoryCredential { get; set; }

        internal ContainerHealthConfigDescription HealthConfig { get; set; }

        internal bool AutoRemove { get; set; }

        internal bool RunInteractive { get; set; }

        internal bool IsContainerRoot { get; set; }

        internal string CodePackageName { get; set; }

        internal string ServicePackageActivationId { get; set; }

        internal string PartitionId { get; set; }

        internal IDictionary<string, string> BindMounts { get; set; }

        internal bool RemoveServiceFabricRuntimeAccess { get; set; }

        internal bool UseDefaultRepositoryCredentials { get; set; }

        internal bool UseTokenAuthenticationCredentials { get; set; }

        internal static unsafe ContainerDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerActivationArgs.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_CONTAINER_DESCRIPTION*)nativePtr);

            var containerDescription = new ContainerDescription
            {
                ApplicationName = NativeTypes.FromNativeString(nativeDescription.ApplicationName),
                ServiceName = NativeTypes.FromNativeString(nativeDescription.ServiceName),
                ApplicationId = NativeTypes.FromNativeString(nativeDescription.ApplicationId),
                ContainerName = NativeTypes.FromNativeString(nativeDescription.ContainerName),
                DeploymentFolder = NativeTypes.FromNativeString(nativeDescription.DeploymentFolder),
                NodeWorkFolder = NativeTypes.FromNativeString(nativeDescription.NodeWorkFolder),
                EntryPoint = NativeTypes.FromNativeString(nativeDescription.EntryPoint),
                HostName = NativeTypes.FromNativeString(nativeDescription.HostName),
                GroupContainerName = NativeTypes.FromNativeString(nativeDescription.GroupContainerName),
                IsolationMode = InteropHelpers.FromNativeContainerIsolationMode(nativeDescription.IsolationMode),
                PortBindings = NativeTypes.FromNativeStringPairList(nativeDescription.PortBindings),
                LogConfig = ContainerLogConfigDescription.CreateFromNative(nativeDescription.LogConfig),
                Volumes = ContainerVolumeDescription.CreateFromNative(nativeDescription.Volumes),
                DnsServers = NativeTypes.FromNativeStringList(nativeDescription.DnsServers),
                SecurityOptions = NativeTypes.FromNativeStringList(nativeDescription.SecurityOptions),
                RepositoryCredential = RepositoryCredentialDescription.CreateFromNative(nativeDescription.RepositoryCredential),
                HealthConfig = ContainerHealthConfigDescription.CreateFromNative(nativeDescription.HealthConfig),
                AutoRemove = NativeTypes.FromBOOLEAN(nativeDescription.AutoRemove),
                RunInteractive = NativeTypes.FromBOOLEAN(nativeDescription.RunInteractive),
                IsContainerRoot = NativeTypes.FromBOOLEAN(nativeDescription.IsContainerRoot),
                CodePackageName = NativeTypes.FromNativeString(nativeDescription.CodePackageName),
                ServicePackageActivationId = NativeTypes.FromNativeString(nativeDescription.ServicePackageActivationId),
                PartitionId = NativeTypes.FromNativeString(nativeDescription.PartitionId),
            };

            if(nativeDescription.Reserved != null)
            {
                var nativeContainerDescriptionEx1 = *((NativeTypes.FABRIC_CONTAINER_DESCRIPTION_EX1*)nativeDescription.Reserved);
                containerDescription.UseDefaultRepositoryCredentials = NativeTypes.FromBOOLEAN(nativeContainerDescriptionEx1.UseDefaultRepositoryCredentials);
                containerDescription.Labels = ContainerLabelDescription.CreateFromNative(nativeContainerDescriptionEx1.Labels);
                containerDescription.RemoveServiceFabricRuntimeAccess = NativeTypes.FromBOOLEAN(nativeContainerDescriptionEx1.RemoveServiceFabricRuntimeAccess);
                containerDescription.BindMounts = NativeTypes.FromNativeStringPairList(nativeContainerDescriptionEx1.BindMounts);

                if (nativeContainerDescriptionEx1.Reserved != null)
                {
                    var nativeContainerDescriptionEx2 = *((NativeTypes.FABRIC_CONTAINER_DESCRIPTION_EX2*)nativeContainerDescriptionEx1.Reserved);
                    containerDescription.UseTokenAuthenticationCredentials = NativeTypes.FromBOOLEAN(nativeContainerDescriptionEx2.UseTokenAuthenticationCredentials);

                    if (nativeContainerDescriptionEx2.Reserved != null)
                    {
                        var nativeContainerDescriptionEx3 = *((NativeTypes.FABRIC_CONTAINER_DESCRIPTION_EX3*)nativeContainerDescriptionEx2.Reserved);
                        containerDescription.ContainerNetworkConfig = ContainerNetworkConfigDescription.CreateFromNative(nativeContainerDescriptionEx3.ContainerNetworkConfigDescription);
                    }
                }
            }

            return containerDescription;
        }
    }
}