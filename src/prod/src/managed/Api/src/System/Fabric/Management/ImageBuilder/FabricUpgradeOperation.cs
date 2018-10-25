// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using Fabric.FabricDeployer;

    internal class FabricUpgradeOperation
    {
        private static readonly string TraceType = "FabricUpgradeOperation";
        
        private ImageStoreWrapper imageStoreWrapper;        

        public FabricUpgradeOperation(ImageStoreWrapper storeWrapper)        
        {                        
            this.imageStoreWrapper = storeWrapper;
        }

        public void UpgradeFabric(
            string currentFabricVersion, 
            string targetFabricVersion,
#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            string configurationCsvFilePath,
#endif
            TimeSpan timeout,
            out bool isConfigOnly)
        {
            isConfigOnly = false;

            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "UpgradeFabric started: CurrentFabricVersion:{0}, TargetFabricVersion:{1}, Timeout:{2}",
                currentFabricVersion,
                targetFabricVersion,
                timeoutHelper.GetRemainingTime());

            // If the current version is invalid (ie. Cluster FabricVersion is not determined)
            // then do not validate
            if (ImageBuilderUtility.Equals(currentFabricVersion, FabricVersion.Invalid.ToString()))
            {
                return;
            }

            FabricVersion currentVersion, targetVersion;

            bool isValid = FabricVersion.TryParse(currentFabricVersion, out currentVersion);
            if (!isValid)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidValue,
                    "CurrentFabricVersion",
                    currentFabricVersion);
            }

            isValid = FabricVersion.TryParse(targetFabricVersion, out targetVersion);
            if (!isValid)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                    TraceType,
                    StringResources.ImageBuilderError_InvalidValue,
                    "TargetFabricVersion",
                    targetFabricVersion);
            }

            WinFabStoreLayoutSpecification winFabLayout = WinFabStoreLayoutSpecification.Create();
            string currentClusterManifestPath = winFabLayout.GetClusterManifestFile(currentVersion.ConfigVersion);
            string targetClusterManifestPath = winFabLayout.GetClusterManifestFile(targetVersion.ConfigVersion);

            ClusterManifestType currentClusterManifest = this.imageStoreWrapper.GetFromStore<ClusterManifestType>(currentClusterManifestPath, timeoutHelper.GetRemainingTime());
            ClusterManifestType targetClusterManifest = this.imageStoreWrapper.GetFromStore<ClusterManifestType>(targetClusterManifestPath, timeoutHelper.GetRemainingTime());

            timeoutHelper.ThrowIfExpired();

            try
            {
                // todo: Vaishnav to pass node list object loaded from the location
                FabricValidator fabricValidator = FabricValidator.Create(currentClusterManifest, null, new WindowsFabricSettings(currentClusterManifest), DeploymentOperations.Update);
                IEnumerable<KeyValuePair<string, string>> modifiedStaticSettings = fabricValidator.CompareAndAnalyze(targetClusterManifest, null /*Detect changes for any NodeType*/);
                isConfigOnly = !modifiedStaticSettings.Any();
            }
            catch (Exception e)
            {
                ImageBuilderUtility.TraceAndThrowValidationError(
                   TraceType,
                   StringResources.ImageBuilderError_InvalidConfigFabricUpgrade,
                   e.ToString(),
                   currentFabricVersion,
                   targetFabricVersion);
            }

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "UpgradeFabric completed: CurrentFabricVersion:{0}, TargetFabricVersion:{1}, ConfigOnly:{2}",
                currentFabricVersion,
                targetFabricVersion,
                isConfigOnly);
        }
    }
}