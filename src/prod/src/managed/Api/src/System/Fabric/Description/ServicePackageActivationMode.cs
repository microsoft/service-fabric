// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;
    using System.Fabric.Query;

    /// <summary>
    /// <para>
    /// Describes service package activation mode for a Service Fabric service. This is specified at the time of
    /// creating the Service (using <see cref="System.Fabric.FabricClient.ServiceManagementClient.CreateServiceAsync(System.Fabric.Description.ServiceDescription)"/>)
    /// or ServiceGroup (using <see cref="System.Fabric.FabricClient.ServiceGroupManagementClient.CreateServiceGroupAsync(System.Fabric.Description.ServiceGroupDescription)"/>) via
    /// <see cref="System.Fabric.Description.ServiceDescription.ServicePackageActivationMode"/>.
    /// </para>
    /// <para>
    /// If no value is specified while creating the Service or ServiceGroup, then it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> mode.
    /// </para>
    /// </summary>
    /// <remarks>
    /// <para>
    /// Consider an example where you have an ApplicationType 'AppTypeA' which contains ServicePackage 'ServicePackageA' which registers
    /// 'ServiceTypeA' and you create many Service(s) of 'ServiceTypeA'. Say 'fabric:/App1_of_AppTypeA/Serv_1' to 
    /// 'fabric:/App1_of_AppTypeA/Serv_N' with ServicePackageActivation mode <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> and 'fabric:/App1_of_AppTypeA/Serv_11' 
    /// to 'fabric:/App1_of_AppTypeA/Serv_NN' with ServicePackageActivation mode <see cref="System.Fabric.Description.ServicePackageActivationMode.ExclusiveProcess"/>.
    /// </para>
    /// <para>
    /// On a given node, replica (or instance) of service 'fabric:/App1_of_AppTypeA/Serv_1' to 'fabric:/App1_of_AppTypeA/Serv_N' will be placed
    /// inside the same activation of 'ServicePackageA' whose <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> will always
    /// be an empty string. However, replica (or instance) of each of  'fabric:/App1_of_AppTypeA/Serv_11' to 'fabric:/App1_of_AppTypeA/Serv_NN'
    /// will be placed in its own dedicated activation of 'ServicePackageA' and each of these activation will have a unique non-empty string as
    /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/>.
    /// </para>
    /// <para>
    /// After you have created your service, you can obtain <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of activated 
    /// ServicePackage(s) on a node by querying <see cref="System.Fabric.Query.DeployedServicePackageList"/> on that node using
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/>.
    /// </para>
    /// </remarks>
    public enum ServicePackageActivationMode
    {
        /// <summary>
        /// <para>
        /// This is the default activation mode. With this activation mode, replica(s) or instance(s) from
        /// different partition(s) of service, on a given node, will share same activation of service package on a node.
        /// </para>
        /// </summary>
        SharedProcess = NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE.FABRIC_ISOLATION_LEVEL_SHARED_PROCESS,
        
        /// <summary>
        /// <para>
        /// With this activation mode, each replica or instance of service, on a given node, will have its own dedicated activation
        /// of service package on a node.
        /// </para>
        /// </summary>
        ExclusiveProcess = NativeTypes.FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE.FABRIC_ISOLATION_LEVEL_EXCLUSIVE_PROCESS
    }
}