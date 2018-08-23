// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Threading.Tasks;

    abstract class ApplicationInstanceOperationBase
    {
        private static readonly string TraceType = "ApplicationInstanceOperationBase";

        protected string ApplicationTypeName { get; private set; }
        protected string ApplicationTypeVersion { get; private set; }
        protected string ApplicationId { get; private set; }        
        protected IDictionary<string, string> UserParameters;

        protected ImageStoreWrapper ImageStoreWrapper { get; private set; }
        protected IEnumerable<IApplicationValidator> Validators { get; private set; }        

        protected ApplicationInstanceOperationBase(
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,            
            IDictionary<string, string> userParameters,
            ImageStoreWrapper imageStoreWrapper,
            IEnumerable<IApplicationValidator> validators)
        {
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ApplicationId = applicationId;            
            this.UserParameters = userParameters;

            this.ImageStoreWrapper = imageStoreWrapper;
            this.Validators = validators;
        }

        protected async Task<ApplicationInstanceContext> CreateAndSortInstanceAsync(
            int appInstanceVersion, 
            Uri nameUri,
            TimeoutHelper timeoutHelper,
            ApplicationTypeContext validatedApplicationTypeContext = null)
        {
            ApplicationTypeContext applicationTypeContext = (validatedApplicationTypeContext == null)
                ? await this.GetApplicationTypeContextAsync(timeoutHelper)
                : validatedApplicationTypeContext;

            IDictionary<string, string> validatedParameters = VaidateAndMergeParameters(this.UserParameters, applicationTypeContext.ApplicationManifest.Parameters);

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Creating ApplicationInstance, ApplicationPackage and ServicePackages from ApplicationManifest and ServiceManifest. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}.",
                this.ApplicationTypeName,
                this.ApplicationTypeVersion);

            RolloutVersion defaultRolloutVersion = RolloutVersion.CreateRolloutVersion();

            ApplicationInstanceBuilder applicationInstanceBuilder = new ApplicationInstanceBuilder(applicationTypeContext, validatedParameters, ImageStoreWrapper, timeoutHelper);
            ApplicationPackageType applicationPackage = applicationInstanceBuilder.BuildApplicationPackage(this.ApplicationId, nameUri, defaultRolloutVersion.ToString());
            ApplicationInstanceType applicationInstance = applicationInstanceBuilder.BuildApplicationInstanceType(this.ApplicationId, nameUri, appInstanceVersion, defaultRolloutVersion.ToString());
            IEnumerable<ServicePackageType> servicePackages = applicationInstanceBuilder.BuildServicePackage(RolloutVersion.CreateRolloutVersion(appInstanceVersion).ToString());

            // Sort parameterizable elements in ApplicationPackage
            ImageBuilderSorterUtility.SortApplicationPackageType(applicationPackage);

            return new ApplicationInstanceContext(
                applicationInstance,
                applicationPackage,
                servicePackages,
                validatedParameters);            
        }        

        protected async Task UploadInstanceAsync(
            ApplicationInstanceType applicationInstanceType,
            ApplicationPackageType applicationPackageType,
            IEnumerable<ServicePackageType> servicePackages,
            StoreLayoutSpecification storeLayoutSpecification /* If null, don't upload to store */,
            StoreLayoutSpecification clusterManagerOutputSpecification /* If null, dont write to output folder*/,
            bool shouldOverwrite,
            TimeoutHelper timeoutHelper)
        {
            List<Task> uploadTasks = new List<Task>();
            // Write ApplicationInstance to Store            
            if (storeLayoutSpecification != null)
            {
                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                    "Starting to upload the ApplicationInstance, ApplicationPackage, ServicePackage to the store. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}.",
                    applicationInstanceType.ApplicationTypeName,
                    applicationInstanceType.ApplicationTypeVersion);

                string applicationInstanceLocation = storeLayoutSpecification.GetApplicationInstanceFile(
                    applicationInstanceType.ApplicationTypeName,
                    applicationInstanceType.ApplicationId,
                    applicationInstanceType.Version.ToString(CultureInfo.InvariantCulture));
                var uploadInstanceTask = this.ImageStoreWrapper.SetToStoreAsync<ApplicationInstanceType>(applicationInstanceLocation, applicationInstanceType, timeoutHelper.GetRemainingTime(), shouldOverwrite);

                uploadTasks.Add(uploadInstanceTask);

                string applicationPackageLocation = storeLayoutSpecification.GetApplicationPackageFile(
                    applicationPackageType.ApplicationTypeName,
                    applicationPackageType.ApplicationId,
                    applicationPackageType.RolloutVersion);
                var uploadApplicationPackageTask = this.ImageStoreWrapper.SetToStoreAsync<ApplicationPackageType>(applicationPackageLocation, applicationPackageType, timeoutHelper.GetRemainingTime(), shouldOverwrite);

                uploadTasks.Add(uploadApplicationPackageTask);
                TimeSpan remainingTime = timeoutHelper.GetRemainingTime();
                foreach (ServicePackageType servicePackage in servicePackages)
                {
                    // Write ServicePackage to Store
                    string servicePackageLocation = storeLayoutSpecification.GetServicePackageFile(
                        applicationInstanceType.ApplicationTypeName,
                        applicationInstanceType.ApplicationId,
                        servicePackage.Name,
                        servicePackage.RolloutVersion);
                    var uploadServicePackageTask = this.ImageStoreWrapper.SetToStoreAsync<ServicePackageType>(servicePackageLocation, servicePackage, remainingTime, shouldOverwrite);

                    uploadTasks.Add(uploadServicePackageTask);
                }

                await Task.WhenAll(uploadTasks);
            }

            // Write ApplicationInstance to the outputFolder for the CM            
            if (clusterManagerOutputSpecification != null)
            {
                ImageBuilder.TraceSource.WriteInfo(
                    TraceType,
                    "Starting to write the ApplicationInstance, ApplicationPackage, ServicePackage to folder {0}. ApplicationTypeName:{1}, ApplicationTypeVersion:{2}.",
                    clusterManagerOutputSpecification.GetRoot(),
                    applicationInstanceType.ApplicationTypeName,
                    applicationInstanceType.ApplicationTypeVersion);

                string clusterManagerApplicationInstanceLocation = clusterManagerOutputSpecification.GetApplicationInstanceFile(
                    applicationInstanceType.ApplicationTypeName,
                    applicationInstanceType.ApplicationId,
                    applicationInstanceType.Version.ToString(CultureInfo.InvariantCulture));
                ImageBuilderUtility.WriteXml<ApplicationInstanceType>(clusterManagerApplicationInstanceLocation, applicationInstanceType);

                string clusterManagerApplicationPackageLocation = clusterManagerOutputSpecification.GetApplicationPackageFile(
                    applicationPackageType.ApplicationTypeName,
                    applicationPackageType.ApplicationId,
                    applicationPackageType.RolloutVersion);
                ImageBuilderUtility.WriteXml<ApplicationPackageType>(clusterManagerApplicationPackageLocation, applicationPackageType);

                foreach (ServicePackageType servicePackage in servicePackages)
                {
                    string clusterManagerServicePackageLocation = clusterManagerOutputSpecification.GetServicePackageFile(
                        applicationInstanceType.ApplicationTypeName,
                        applicationInstanceType.ApplicationId,
                        servicePackage.Name,
                        servicePackage.RolloutVersion);
                    ImageBuilderUtility.WriteXml<ServicePackageType>(clusterManagerServicePackageLocation, servicePackage);
                }
            }

            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Completed uploading/writing ApplicationInstance, ApplicationPackage, ServicePackage to the store/folder. ApplicationTypeName:{0}, ApplicationTypeVersion:{1}.",
                applicationInstanceType.ApplicationTypeName,
                applicationInstanceType.ApplicationTypeVersion);
        }        

        protected void ValidateApplicationInstance(ApplicationInstanceContext appInstanceContext)
        {
            foreach (IApplicationValidator validator in this.Validators)
            {
                validator.Validate(appInstanceContext);
            }
        }        

        private async Task<ApplicationTypeContext> GetApplicationTypeContextAsync(TimeoutHelper timeoutHelper)
        {
            StoreLayoutSpecification storeLayoutSpecification = StoreLayoutSpecification.Create();

            // Read the the ApplicationManifest from the store            
            string applicationManifestFile = storeLayoutSpecification.GetApplicationManifestFile(this.ApplicationTypeName, this.ApplicationTypeVersion);
            ApplicationManifestType applicationManifestType = await this.ImageStoreWrapper.GetFromStoreAsync<ApplicationManifestType>(applicationManifestFile, timeoutHelper.GetRemainingTime());

            ApplicationTypeContext applicationTypeContext = new ApplicationTypeContext(applicationManifestType, string.Empty);
            
            // Read the Collection of ServiceManifests associated with the ApplicationManifest from the store
            List<Task<ServiceManifest>> taskList = new List<Task<ServiceManifest>>();
            TimeSpan remainingTime = timeoutHelper.GetRemainingTime();
            foreach (ApplicationManifestTypeServiceManifestImport serviceManifestImport in applicationManifestType.ServiceManifestImport)
            {
                taskList.Add(this.GetServiceManifestAsync(serviceManifestImport, storeLayoutSpecification, remainingTime));
            }

            await Task.WhenAll(taskList);

            taskList.ForEach(task => applicationTypeContext.ServiceManifests.Add(task.Result));

            return applicationTypeContext;
        }

        private async Task<ServiceManifest> GetServiceManifestAsync(
            ApplicationManifestTypeServiceManifestImport serviceManifestImport,             
            StoreLayoutSpecification storeLayoutSpecification,
            TimeSpan timeout)
        {
            TimeoutHelper timeoutHelper = new TimeoutHelper(timeout);
            string serviceManifestFile = storeLayoutSpecification.GetServiceManifestFile(
                    this.ApplicationTypeName,
                    serviceManifestImport.ServiceManifestRef.ServiceManifestName,
                    serviceManifestImport.ServiceManifestRef.ServiceManifestVersion);

            string checksumFile = storeLayoutSpecification.GetServiceManifestChecksumFile(
                this.ApplicationTypeName,
                serviceManifestImport.ServiceManifestRef.ServiceManifestName,
                serviceManifestImport.ServiceManifestRef.ServiceManifestVersion);

            var getServiceManifestTask = this.ImageStoreWrapper.GetFromStoreAsync<ServiceManifestType>(serviceManifestFile, timeoutHelper.GetRemainingTime());
            var getChecksumTask = this.ImageStoreWrapper.TryGetFromStoreAsync(checksumFile, timeoutHelper.GetRemainingTime());

            await Task.WhenAll(getServiceManifestTask, getChecksumTask);

            timeoutHelper.ThrowIfExpired();
            ServiceManifestType serviceManifestType = getServiceManifestTask.Result;
            string checksum = getChecksumTask.Result.Item1;

            ServiceManifest serviceManifest = new ServiceManifest(serviceManifestType) { Checksum = checksum };

            List<Task<CodePackage>> codePackageTaskList = new List<Task<CodePackage>>();
            TimeSpan remainingTime = timeoutHelper.GetRemainingTime();
            foreach (CodePackageType codePackageType in serviceManifestType.CodePackage)
            {
                codePackageTaskList.Add(this.GetCodePackageAsync(codePackageType, serviceManifest, storeLayoutSpecification, remainingTime));
            }

            List<Task<ConfigPackage>> configPackageTaskList = new List<Task<ConfigPackage>>();
            if (serviceManifestType.ConfigPackage != null)
            {
                foreach (ConfigPackageType configPackageType in serviceManifestType.ConfigPackage)
                {
                    configPackageTaskList.Add(this.GetConfigPackageAsync(configPackageType, serviceManifest, storeLayoutSpecification, remainingTime));
                }
            }

            List<Task<DataPackage>> dataPackageTaskList = new List<Task<DataPackage>>();
            if (serviceManifestType.DataPackage != null)
            {
                foreach (DataPackageType dataPackageType in serviceManifestType.DataPackage)
                {
                    dataPackageTaskList.Add(this.GetDataPackageAsync(dataPackageType, serviceManifest, storeLayoutSpecification, remainingTime));
                }
            }

            List<Task> packageTasks = new List<Task>();
            packageTasks.AddRange(codePackageTaskList);
            packageTasks.AddRange(configPackageTaskList);
            packageTasks.AddRange(dataPackageTaskList);

            await Task.WhenAll(packageTasks);

            codePackageTaskList.ForEach(task => serviceManifest.CodePackages.Add(task.Result));
            configPackageTaskList.ForEach(task => serviceManifest.ConfigPackages.Add(task.Result));
            dataPackageTaskList.ForEach(task => serviceManifest.DataPackages.Add(task.Result));

            return serviceManifest;
        }

        private async Task<CodePackage> GetCodePackageAsync(
            CodePackageType codePackageType, 
            ServiceManifest serviceManifest, 
            StoreLayoutSpecification storeLayoutSpecification,
            TimeSpan timeout)
        {
            var checksumFile = storeLayoutSpecification.GetCodePackageChecksumFile(
                    this.ApplicationTypeName,
                    serviceManifest.ServiceManifestType.Name,
                    codePackageType.Name,
                    codePackageType.Version);
            var checksumTask = await this.ImageStoreWrapper.TryGetFromStoreAsync(checksumFile, timeout);

            return new CodePackage(codePackageType) { Checksum = checksumTask.Item1 /*checksumValue*/ };
        }

        private async Task<ConfigPackage> GetConfigPackageAsync(            
            ConfigPackageType configPackageType,
            ServiceManifest serviceManifest,
            StoreLayoutSpecification storeLayoutSpecification,
            TimeSpan timeout)
        {
            var checksumFile = storeLayoutSpecification.GetConfigPackageChecksumFile(
                    this.ApplicationTypeName,
                    serviceManifest.ServiceManifestType.Name,
                    configPackageType.Name,
                    configPackageType.Version);
            var checksumTask = await this.ImageStoreWrapper.TryGetFromStoreAsync(checksumFile, timeout);

            return new ConfigPackage(configPackageType) { Checksum = checksumTask.Item1 /*checksumValue*/ };
        }

        private async Task<DataPackage> GetDataPackageAsync(
            DataPackageType dataPackageType,
            ServiceManifest serviceManifest,
            StoreLayoutSpecification storeLayoutSpecification,
            TimeSpan timeout)
        {
            var checksumFile = storeLayoutSpecification.GetDataPackageChecksumFile(
                    this.ApplicationTypeName,
                    serviceManifest.ServiceManifestType.Name,
                    dataPackageType.Name,
                    dataPackageType.Version);
            var checksumTask = await this.ImageStoreWrapper.TryGetFromStoreAsync(checksumFile, timeout);

            return new DataPackage(dataPackageType) { Checksum = checksumTask.Item1 /*checksumValue*/ };
        }

        private static IDictionary<string, string> VaidateAndMergeParameters(IDictionary<string, string> userParameters, ApplicationManifestTypeParameter[] applicationParameters)
        {
            ImageBuilder.TraceSource.WriteInfo(
                TraceType,
                "Validating the user parameters passed for the Application.");
            IDictionary<string, string> validatedParameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            IDictionary<string, string> applicationParametersDictionary = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            if (applicationParameters != null)
            {
                foreach (ApplicationManifestTypeParameter applicationParameter in applicationParameters)
                {
                    applicationParametersDictionary.Add(applicationParameter.Name, applicationParameter.DefaultValue);
                }
            }

            if (userParameters != null)
            {
                foreach (KeyValuePair<string, string> userParameter in userParameters)
                {
                    if (!applicationParametersDictionary.ContainsKey(userParameter.Key) &&
                        !userParameter.Key.Equals(ApplicationInstanceBuilder.DebugParameter))
                    {
                        throw new FabricImageBuilderValidationException(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "Parameter '{0}' is not defined in the ApplicationManifest file.",
                                userParameter.Key),
                             "applicationParameters");
                    }

                    if (validatedParameters.ContainsKey(userParameter.Key))
                    {
                        throw new FabricImageBuilderValidationException(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "Parameter '{0}' is provided more than once. Duplicate parameters are not allowed.",
                                userParameter.Key),
                             "applicationParameters");
                    }

                    validatedParameters.Add(userParameter.Key, userParameter.Value);
                }
            }

            foreach (KeyValuePair<string, string> applicationParameter in applicationParametersDictionary)
            {
                if (!validatedParameters.ContainsKey(applicationParameter.Key))
                {
                    validatedParameters.Add(applicationParameter.Key, applicationParameter.Value);
                }
            }

            return validatedParameters;
        }
    }
}