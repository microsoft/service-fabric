// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;

    internal class ValidateOperation : DeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType targetClusterManifest, Infrastructure infrastructure)
        {
            ClusterManifestType currentClusterManifest;
            InfrastructureInformationType currentInfrastructureManifest;

            //Powershell Test-ServiceFabricClusterManifest Command
            if (!string.IsNullOrEmpty(parameters.OldClusterManifestLocation)) 
            {
                currentClusterManifest = XmlHelper.ReadXml<ClusterManifestType>(parameters.OldClusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
                currentInfrastructureManifest = parameters.InfrastructureManifestLocation == null ?
                    null :
                    XmlHelper.ReadXml<InfrastructureInformationType>(parameters.InfrastructureManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
            }
            else //Hosting2 Fabric Upgrade Diff Validation
            {
                string currentClusterManifestPath = parameters.DeploymentSpecification.GetCurrentClusterManifestFile(parameters.NodeName);
	            string infrastructureManifestPath = parameters.DeploymentSpecification.GetInfrastructureManfiestFile(parameters.NodeName);
                currentClusterManifest = XmlHelper.ReadXml<ClusterManifestType>(currentClusterManifestPath, SchemaLocation.GetWindowsFabricSchemaLocation());
                currentInfrastructureManifest = XmlHelper.ReadXml<InfrastructureInformationType>(infrastructureManifestPath, SchemaLocation.GetWindowsFabricSchemaLocation());
            }

            //Update to the newest infrastructure
            infrastructure = targetClusterManifest == null ? null : Infrastructure.Create(targetClusterManifest.Infrastructure, currentInfrastructureManifest == null ? null : currentInfrastructureManifest.NodeList, targetClusterManifest.NodeTypes);
            FabricValidatorWrapper.CompareAndAnalyze(currentClusterManifest, targetClusterManifest, infrastructure, parameters);
        }
    }
}