// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;
    using System.Threading.Tasks;

    class ApplicationUpgradeInstanceOperation : ApplicationInstanceOperationBase
    {
        private static readonly string TraceType = "ApplicationUpgradeInstanceOperation";

        private int currentApplicationInstanceVersion;

        public ApplicationUpgradeInstanceOperation(
            int currentApplicationInstanceVersion,
            string applicationTypeName,
            string targetAppTypeVersion,
            string applicationId,
            IDictionary<string, string> userParameters,
            ImageStoreWrapper imageStoreWrapper,
            IEnumerable<IApplicationValidator> validators)
            : base(applicationTypeName, targetAppTypeVersion, applicationId, userParameters, imageStoreWrapper, validators)
        {
            this.currentApplicationInstanceVersion = currentApplicationInstanceVersion;
        }

        public async Task UpgradeInstaceAsync(string outputFolder, TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Starting UpgradeInstace. ApplicationTypeName:{0}, TargetApplicationTypeVersion:{1}, CurrentApplicationInstance:{2}, ApplicationId:{3}, Timeout:{4}",
                this.ApplicationTypeName,
                this.ApplicationTypeVersion,
                this.currentApplicationInstanceVersion,
                this.ApplicationId,
                timeoutHelper.GetRemainingTime());

            StoreLayoutSpecification storeLayoutSpecification = StoreLayoutSpecification.Create();

            // Read the current ApplicationInstance and ApplicationPackage from the store
            string currentApplicationInstanceFile = storeLayoutSpecification.GetApplicationInstanceFile(this.ApplicationTypeName, this.ApplicationId, this.currentApplicationInstanceVersion.ToString(CultureInfo.InvariantCulture));
            ApplicationInstanceType currentApplicationInstanceType = this.ImageStoreWrapper.GetFromStore<ApplicationInstanceType>(currentApplicationInstanceFile, timeoutHelper.GetRemainingTime());

            string currentApplicationPackageFile = storeLayoutSpecification.GetApplicationPackageFile(this.ApplicationTypeName, this.ApplicationId, currentApplicationInstanceType.ApplicationPackageRef.RolloutVersion);
            ApplicationPackageType currentApplicationPackageType = this.ImageStoreWrapper.GetFromStore<ApplicationPackageType>(currentApplicationPackageFile, timeoutHelper.GetRemainingTime());

            // Read the current ServicePackages from the store
            List<Task<ServicePackageType>> getServicePackageTasks = new List<Task<ServicePackageType>>();
            TimeSpan remainingTime = timeoutHelper.GetRemainingTime();
            foreach (ApplicationInstanceTypeServicePackageRef servicePackageRef in currentApplicationInstanceType.ServicePackageRef)
            {
                string currentServicePackageFile = storeLayoutSpecification.GetServicePackageFile(
                            this.ApplicationTypeName,
                            this.ApplicationId,
                            servicePackageRef.Name,
                            servicePackageRef.RolloutVersion);
                var getServicePackageTask = this.ImageStoreWrapper.GetFromStoreAsync<ServicePackageType>(currentServicePackageFile, remainingTime);
                getServicePackageTasks.Add(getServicePackageTask);
            }

            await Task.WhenAll(getServicePackageTasks);

            Collection<ServicePackageType> currentServicePackages = new Collection<ServicePackageType>();
            getServicePackageTasks.ForEach(task => currentServicePackages.Add(task.Result));

            timeoutHelper.ThrowIfExpired();

            ApplicationInstanceContext targetAppInstanceContext = await base.CreateAndSortInstanceAsync(
                currentApplicationInstanceType.Version + 1,
                new Uri(currentApplicationPackageType.NameUri),
                timeoutHelper);

            // Validate the target ApplicationInstance and ServicePackages
            this.ValidateApplicationInstance(targetAppInstanceContext);

            // Update the Rollout version on the target ApplicationInstance
            this.UpdateTargetApplicationPackage(currentApplicationPackageType, targetAppInstanceContext.ApplicationPackage);
            targetAppInstanceContext.ApplicationInstance.ApplicationPackageRef.RolloutVersion = targetAppInstanceContext.ApplicationPackage.RolloutVersion;

            // Update the Rollout version on the target ServicePackages
            foreach (ServicePackageType targetServicePackage in targetAppInstanceContext.ServicePackages)
            {
                ServicePackageType matchingCurrentServicePackageType = currentServicePackages.FirstOrDefault(
                    currentServicePackage => ImageBuilderUtility.Equals(currentServicePackage.Name, targetServicePackage.Name));

                this.UpdateTargetServicePackage(matchingCurrentServicePackageType, targetServicePackage);

                ApplicationInstanceTypeServicePackageRef matchingServicePackageRef = targetAppInstanceContext.ApplicationInstance.ServicePackageRef.First(
                    servicePackageRef => ImageBuilderUtility.Equals(servicePackageRef.Name, targetServicePackage.Name));
                matchingServicePackageRef.RolloutVersion = targetServicePackage.RolloutVersion;
            }

            StoreLayoutSpecification clusterManagerOutputSpecification = null;
            if (outputFolder != null)
            {
                clusterManagerOutputSpecification = StoreLayoutSpecification.Create();
                clusterManagerOutputSpecification.SetRoot(outputFolder);
            }

            timeoutHelper.ThrowIfExpired();

            // Upload the target ApplicationInstance and ServicePackages to the store
            // Also, write the target ApplicationInstance and ServicePackages to the CM output folder
            await this.UploadInstanceAsync(
                targetAppInstanceContext.ApplicationInstance,
                targetAppInstanceContext.ApplicationPackage,
                targetAppInstanceContext.ServicePackages,
                storeLayoutSpecification,
                clusterManagerOutputSpecification,
                false,
                timeoutHelper);

            // Write the current ApplicationInstance and ServicePackages to the CM output folder
            await this.UploadInstanceAsync(
                currentApplicationInstanceType,
                currentApplicationPackageType,
                currentServicePackages,
                null /* Do not upload to store*/,
                clusterManagerOutputSpecification,
                true,
                timeoutHelper);

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Completed UpgradeInstace. ApplicationTypeName:{0}, TargetApplicationTypeVersion:{1}, CurrentApplicationInstance:{2}, ApplicationId:{3}",
                this.ApplicationTypeName,
                this.ApplicationTypeVersion,
                this.currentApplicationInstanceVersion,
                this.ApplicationId);
        }

        private void UpdateTargetServicePackage(ServicePackageType currentServicePackage, ServicePackageType targetServicePackage)
        {
            if (currentServicePackage == null)
            {
                // New ServicePackage added
                return;
            }

            IntializeTargetPackageWithCurrentRolloutVersion(currentServicePackage, targetServicePackage);

            var hasServiceTypesChanged = ImageBuilderUtility.IsNotEqual(
                currentServicePackage.DigestedServiceTypes.ServiceTypes,
                targetServicePackage.DigestedServiceTypes.ServiceTypes);

            var hasResourcesChanged = ImageBuilderUtility.IsNotEqual(
                currentServicePackage.DigestedResources.DigestedEndpoints,
                targetServicePackage.DigestedResources.DigestedEndpoints);
            if (!hasResourcesChanged)
            {
                hasResourcesChanged =
                    ImageBuilderUtility.IsNotEqual(
                        currentServicePackage.DigestedResources.DigestedCertificates,
                        targetServicePackage.DigestedResources.DigestedCertificates);
            }

            var hasDiagnosticsChanged = ImageBuilderUtility.IsNotEqual(
                currentServicePackage.Diagnostics,
                targetServicePackage.Diagnostics);

            var hasCodePackagesChanged = ImageBuilderUtility.IsNotEqual(
  	                currentServicePackage.DigestedCodePackage,
  	                targetServicePackage.DigestedCodePackage);

            var hasConfigPackagesChanged = ImageBuilderUtility.IsNotEqual(
                currentServicePackage.DigestedConfigPackage,
                targetServicePackage.DigestedConfigPackage);

            var hasDataPackagesChanged = ImageBuilderUtility.IsNotEqual(
                currentServicePackage.DigestedDataPackage,
                targetServicePackage.DigestedDataPackage);

            var requiresContainerGroupSetup = RequiresContainerGroupSetup(
                currentServicePackage, 
                targetServicePackage);

            var isOnDemandActivationImpacted = false;
            if(hasCodePackagesChanged)
            {
                isOnDemandActivationImpacted = this.IsOnDemandCodePackageActivationImapcted(
                    currentServicePackage.DigestedCodePackage,
                    targetServicePackage.DigestedCodePackage);
            }

            // Compute the target RolloutVersion
            var currentRolloutVersion = RolloutVersion.CreateRolloutVersion(currentServicePackage.RolloutVersion);
            RolloutVersion targetRolloutVersion = null;

            if (hasResourcesChanged || hasServiceTypesChanged || requiresContainerGroupSetup || isOnDemandActivationImpacted)
            {
                targetRolloutVersion = currentRolloutVersion.NextMajorRolloutVersion();
            }
            else if (hasCodePackagesChanged || hasConfigPackagesChanged || hasDataPackagesChanged || hasDiagnosticsChanged)
            {
                targetRolloutVersion = currentRolloutVersion.NextMinorRolloutVersion();
            }

            // Update the RolloutVersion on the target ServicePackage
            string targetRolloutVerionString = (targetRolloutVersion != null) ? targetRolloutVersion.ToString() : null;

            targetServicePackage.RolloutVersion = (targetRolloutVerionString != null) ? targetRolloutVerionString : currentServicePackage.RolloutVersion;
            targetServicePackage.DigestedServiceTypes.RolloutVersion = hasServiceTypesChanged ? targetRolloutVerionString : currentServicePackage.DigestedServiceTypes.RolloutVersion;
            targetServicePackage.DigestedResources.RolloutVersion = hasResourcesChanged ? targetRolloutVerionString : currentServicePackage.DigestedResources.RolloutVersion;

            foreach (var targetDigestedCodePackage in targetServicePackage.DigestedCodePackage)
            {
                var matchingCurrentDigestedCodePackage = currentServicePackage.DigestedCodePackage.FirstOrDefault(
                    digestedCodePackage => ImageBuilderUtility.Equals(digestedCodePackage.CodePackage.Name, targetDigestedCodePackage.CodePackage.Name));

                if (matchingCurrentDigestedCodePackage != null)
                {
                    targetDigestedCodePackage.RolloutVersion = hasCodePackagesChanged && ImageBuilderUtility.IsNotEqual(matchingCurrentDigestedCodePackage, targetDigestedCodePackage)
                        ? targetRolloutVerionString
                        : matchingCurrentDigestedCodePackage.RolloutVersion;
                }
                else
                {
                    targetDigestedCodePackage.RolloutVersion = targetRolloutVerionString;
                }
            }

            if (targetServicePackage.DigestedConfigPackage != null)
            {
                foreach (ServicePackageTypeDigestedConfigPackage targetDigestedConfigPackage in targetServicePackage.DigestedConfigPackage)
                {
                    bool hasChanged = true;
                    ServicePackageTypeDigestedConfigPackage matchingCurrentDigestedConfigPackage = null;
                    if (currentServicePackage.DigestedConfigPackage != null)
                    {
                        matchingCurrentDigestedConfigPackage = currentServicePackage.DigestedConfigPackage.FirstOrDefault(
                            digestedConfigPackage => ImageBuilderUtility.Equals(digestedConfigPackage.ConfigPackage.Name, targetDigestedConfigPackage.ConfigPackage.Name));

                        if (matchingCurrentDigestedConfigPackage != null)
                        {
                            if (hasConfigPackagesChanged)
                            {
                                hasChanged = ImageBuilderUtility.IsNotEqual<ServicePackageTypeDigestedConfigPackage>(
                                    matchingCurrentDigestedConfigPackage,
                                    targetDigestedConfigPackage);
                            }
                            else
                            {
                                hasChanged = false;
                            }
                        }
                    }

                    targetDigestedConfigPackage.RolloutVersion = hasChanged ? targetRolloutVerionString : matchingCurrentDigestedConfigPackage.RolloutVersion;
                }
            }

            if (targetServicePackage.DigestedDataPackage != null)
            {
                foreach (ServicePackageTypeDigestedDataPackage targetDigestedDataPackage in targetServicePackage.DigestedDataPackage)
                {
                    bool hasChanged = true;
                    ServicePackageTypeDigestedDataPackage matchingCurrentDigestedDataPackage = null;
                    if (currentServicePackage.DigestedDataPackage != null)
                    {
                        matchingCurrentDigestedDataPackage = currentServicePackage.DigestedDataPackage.FirstOrDefault(
                            digestedDataPackage => ImageBuilderUtility.Equals(digestedDataPackage.DataPackage.Name, targetDigestedDataPackage.DataPackage.Name));

                        if (matchingCurrentDigestedDataPackage != null)
                        {
                            if (hasDataPackagesChanged)
                            {
                                hasChanged = ImageBuilderUtility.IsNotEqual<ServicePackageTypeDigestedDataPackage>(
                                    matchingCurrentDigestedDataPackage,
                                    targetDigestedDataPackage);
                            }
                            else
                            {
                                hasChanged = false;
                            }
                        }
                    }

                    targetDigestedDataPackage.RolloutVersion = hasChanged ? targetRolloutVerionString : matchingCurrentDigestedDataPackage.RolloutVersion;
                }
            }
        }       

        private bool RequiresContainerGroupSetup(ServicePackageType currentServicePackage, ServicePackageType targetServicePackage)
        {
            bool requiresSetup = false;
            int currentContainerCount = 0;
            int targetContainerCount = 0;
            foreach(ServicePackageTypeDigestedCodePackage codePackage in currentServicePackage.DigestedCodePackage)
            {
                if(codePackage.CodePackage.EntryPoint.Item is ContainerHostEntryPointType)
                {
                    currentContainerCount++;
                }
            }
            foreach (ServicePackageTypeDigestedCodePackage codePackage in targetServicePackage.DigestedCodePackage)
            {
                if (codePackage.CodePackage.EntryPoint.Item is ContainerHostEntryPointType)
                {
                    targetContainerCount++;
                }
            }
            if(currentContainerCount <= 1 &&
                targetContainerCount >1)
            {
                requiresSetup = true;
            }
            return requiresSetup;
        }

        private bool IsOnDemandCodePackageActivationImapcted(
            ServicePackageTypeDigestedCodePackage[] currentDigestedCodePackages,
            ServicePackageTypeDigestedCodePackage[] targetDigestedCodePackages)
        {
            var currentHasActivatorCodePackage = this.HasActivatorCodePackage(
                currentDigestedCodePackages, out ServicePackageTypeDigestedCodePackage currentActivatorCodePackage);

            var targetHasActivatorCodePackage = this.HasActivatorCodePackage(
                targetDigestedCodePackages, out ServicePackageTypeDigestedCodePackage targetActivatorCodePackage);

            if (!currentHasActivatorCodePackage && !targetHasActivatorCodePackage)
            {
                return false;
            }

            if (currentHasActivatorCodePackage != targetHasActivatorCodePackage ||
                ImageBuilderUtility.IsNotEqual(currentActivatorCodePackage, targetActivatorCodePackage) ||
                currentDigestedCodePackages.Length != targetDigestedCodePackages.Length ||
                this.CheckTargetContainsSource(currentDigestedCodePackages, targetDigestedCodePackages) == false ||
                this.CheckTargetContainsSource(targetDigestedCodePackages, currentDigestedCodePackages) == false)
            {
                return true;
            }

            return false;
        }

        bool CheckTargetContainsSource(
            ServicePackageTypeDigestedCodePackage[] sourceDigestedCodePackages,
            ServicePackageTypeDigestedCodePackage[] targetDigestedCodePackages)
        {
            foreach (var currentDcp in sourceDigestedCodePackages)
            {
                var matchFound = false;

                foreach (var targetDcp in targetDigestedCodePackages)
                {
                    if (currentDcp.CodePackage.Name.Equals(targetDcp.CodePackage.Name, StringComparison.OrdinalIgnoreCase))
                    {
                        matchFound = true;
                        break;
                    }
                }

                if (!matchFound)
                {
                    return false;
                }
            }

            return true;
        }

        private bool HasActivatorCodePackage(
            ServicePackageTypeDigestedCodePackage[] digestedCodePackages,
            out ServicePackageTypeDigestedCodePackage activatorCodePackage)
        {
            activatorCodePackage = null;

            foreach (var dcp in digestedCodePackages)
            {
                if(dcp.CodePackage.IsActivator)
                {
                    activatorCodePackage = dcp;
                    return true;
                }
            }

            return false;
        }

        private void IntializeTargetPackageWithCurrentRolloutVersion(ServicePackageType currentServicePackage, ServicePackageType targetServicePackage)
        {
            //Set target rolloutversion to current before comparison
            for (int i = 0; i < targetServicePackage.DigestedCodePackage.Length; i++)
            {
                ServicePackageTypeDigestedCodePackage targetDigestedCodePackage = targetServicePackage.DigestedCodePackage[i];

                ServicePackageTypeDigestedCodePackage matchingCurrentDigestedCodePackage = currentServicePackage.DigestedCodePackage.FirstOrDefault(
                    digestedCodePackage => ImageBuilderUtility.Equals(digestedCodePackage.CodePackage.Name, targetDigestedCodePackage.CodePackage.Name));

                if (matchingCurrentDigestedCodePackage != null)
                {
                    targetDigestedCodePackage.RolloutVersion = matchingCurrentDigestedCodePackage.RolloutVersion;
                }
            }

            //Set target rolloutversion for config to current before comparison
            if (targetServicePackage.DigestedConfigPackage != null)
            {
                foreach (ServicePackageTypeDigestedConfigPackage targetDigestedConfigPackage in targetServicePackage.DigestedConfigPackage)
                {
                    ServicePackageTypeDigestedConfigPackage matchingCurrentDigestedConfigPackage = null;
                    if (currentServicePackage.DigestedConfigPackage != null)
                    {
                        matchingCurrentDigestedConfigPackage = currentServicePackage.DigestedConfigPackage.FirstOrDefault(
                            digestedConfigPackage => ImageBuilderUtility.Equals(digestedConfigPackage.ConfigPackage.Name, targetDigestedConfigPackage.ConfigPackage.Name));

                        if (matchingCurrentDigestedConfigPackage != null)
                        {
                            targetDigestedConfigPackage.RolloutVersion = matchingCurrentDigestedConfigPackage.RolloutVersion;
                        }
                    }
                }
            }

            //Set RolloutVersion for matching data packages to be same as current
            if (targetServicePackage.DigestedDataPackage != null)
            {
                foreach (ServicePackageTypeDigestedDataPackage targetDigestedDataPackage in targetServicePackage.DigestedDataPackage)
                {
                    ServicePackageTypeDigestedDataPackage matchingCurrentDigestedDataPackage = null;
                    if (currentServicePackage.DigestedDataPackage != null)
                    {
                        matchingCurrentDigestedDataPackage = currentServicePackage.DigestedDataPackage.FirstOrDefault(
                            digestedDataPackage => ImageBuilderUtility.Equals(digestedDataPackage.DataPackage.Name, targetDigestedDataPackage.DataPackage.Name));

                        if (matchingCurrentDigestedDataPackage != null)
                        {
                            targetDigestedDataPackage.RolloutVersion = matchingCurrentDigestedDataPackage.RolloutVersion;
                        }
                    }
                }
            }
        }
        
        private void UpdateTargetApplicationPackage(ApplicationPackageType currentApplicationPackageType, ApplicationPackageType targetApplicationPackageType)
        {
            bool hasPoliciesChanged = ImageBuilderUtility.IsNotEqual<ApplicationPoliciesType>(
                currentApplicationPackageType.DigestedEnvironment.Policies,
                targetApplicationPackageType.DigestedEnvironment.Policies);

            bool hasPrincipalsChanged = ImageBuilderUtility.IsNotEqual<SecurityPrincipalsType>(
                currentApplicationPackageType.DigestedEnvironment.Principals,
                targetApplicationPackageType.DigestedEnvironment.Principals);

            bool hasDiagnosticsChanged = ImageBuilderUtility.IsNotEqual<DiagnosticsType>(
                currentApplicationPackageType.DigestedEnvironment.Diagnostics,
                targetApplicationPackageType.DigestedEnvironment.Diagnostics);

            bool hasApplicationEnvironmentChanged = hasPoliciesChanged || hasPrincipalsChanged || hasDiagnosticsChanged;

            bool hasSecretsCertificateChanged = ImageBuilderUtility.IsNotEqual<FabricCertificateType>(
                currentApplicationPackageType.DigestedCertificates.SecretsCertificate,
                targetApplicationPackageType.DigestedCertificates.SecretsCertificate);

            bool hasEndpointCertificateChanged =
                ImageBuilderUtility.IsNotEqual<EndpointCertificateType>(
                    currentApplicationPackageType.DigestedCertificates.EndpointCertificate,
                    targetApplicationPackageType.DigestedCertificates.EndpointCertificate);

            // Computer the target RolloutVersion
            RolloutVersion currentRolloutVersion = RolloutVersion.CreateRolloutVersion(currentApplicationPackageType.RolloutVersion);
            RolloutVersion targetRolloutVersion = currentRolloutVersion;
            if (hasApplicationEnvironmentChanged || hasSecretsCertificateChanged || hasEndpointCertificateChanged)
            {
                targetRolloutVersion = currentRolloutVersion.NextMajorRolloutVersion();
            }

            // Update the RolloutVersion on the target ApplicationInstance
            targetApplicationPackageType.DigestedEnvironment.RolloutVersion =
                hasApplicationEnvironmentChanged ? targetRolloutVersion.ToString() : currentApplicationPackageType.DigestedEnvironment.RolloutVersion;

            targetApplicationPackageType.DigestedCertificates.RolloutVersion =
                (hasSecretsCertificateChanged || hasEndpointCertificateChanged) ? targetRolloutVersion.ToString() : currentApplicationPackageType.DigestedCertificates.RolloutVersion;

            targetApplicationPackageType.RolloutVersion = targetRolloutVersion.ToString();
        }
    }
}