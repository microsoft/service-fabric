// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.ImageStore;
    using System.Fabric.Management;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Management.Automation;
    using System.Text;

    public abstract class ApplicationCmdletBase : CommonCmdletBase
    {
        internal NameValueCollection GetNameValueCollection(Hashtable parameters)
        {
            var appParameterCollection = new NameValueCollection();

            if (parameters == null)
            {
                return appParameterCollection;
            }

            try
            {
                foreach (var key in parameters.Keys)
                {
                    appParameterCollection.Add((string)key, (string)parameters[key]);
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.GetNameValueCollectionErrorId,
                    appParameterCollection);
            }

            return appParameterCollection;
        }

        internal IDictionary<string, string> GetDictionary(Hashtable parameters)
        {
            if (parameters == null)
            {
                return null;
            }

            var appParameterCollection = new Dictionary<string, string>();

            try
            {
                foreach (var key in parameters.Keys)
                {
                    appParameterCollection.Add((string)key, (string)parameters[key]);
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.GetNameValueCollectionErrorId,
                    appParameterCollection);
            }

            return appParameterCollection;
        }

        protected void TestComposeDeploymentPackage(
            string dockerComposeFilePath, 
            string registryUserName, 
            string registryPassword, 
            bool isEncrypted, 
            string imageStoreConnectionString)
        {
            var testRootPath = Path.Combine(Path.GetTempPath(), string.Format(CultureInfo.InvariantCulture, "{0}_{1}", Constants.BaseTestRootFolderName, Stopwatch.GetTimestamp()));

            try
            {
                var composeAbsolutePath = this.GetAbsolutePath(dockerComposeFilePath);
                var fileImageStoreConnectionString =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "{0}{1}",
                        Constants.ImageStoreConnectionFileType,
                        testRootPath.TrimEnd(Path.DirectorySeparatorChar));

                FabricDirectory.CreateDirectory(testRootPath);
                var sourceImageStore = ImageStoreFactoryProxy.CreateImageStore(fileImageStoreConnectionString);

                IImageStore destinationImageStore = null;
                if (string.IsNullOrEmpty(imageStoreConnectionString))
                {
                    destinationImageStore = sourceImageStore;
                }
                else
                {
                    destinationImageStore = this.CreateImageStore(imageStoreConnectionString, testRootPath);
                }

                var imagebuilder = new ImageBuilder(
                    sourceImageStore,
                    destinationImageStore,
                    this.GetFabricFilePath(Constants.ServiceModelSchemaFileName),
                    testRootPath);

                HashSet<string> ignoredKeys;
                imagebuilder.ValidateComposeDeployment(
                    composeAbsolutePath,
                    null,
                    null,
                    "Compose_",
                    DateTime.Now.Ticks.ToString(),
                    TimeSpan.MaxValue,
                    out ignoredKeys,
                    false);

                if (ignoredKeys.Count > 0)
                {
                    StringBuilder sb = new StringBuilder();
                    sb.Append("The Docker compose file contains the following 'keys' which are not supported. They will be ignored. ");
                    sb.Append(Environment.NewLine);
                    foreach (var key in ignoredKeys)
                    {
                        sb.Append(string.Format("'{0}' ", key));
                    }

                    this.WriteWarning(sb.ToString());
                }

                this.WriteObject(true);
            }
            catch (AggregateException exception)
            {
                exception.Handle((ae) =>
                {
                    this.WriteObject(false);
                    this.ThrowTerminatingError(
                        ae,
                        Constants.TestApplicationPackageErrorId,
                        null);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.WriteObject(false);
                this.ThrowTerminatingError(
                    exception,
                    Constants.TestApplicationPackageErrorId,
                    null);
            }
            finally
            {
                FabricDirectory.Delete(testRootPath, true, true);
            }
        }

        protected void TestSFApplicationPackage(string applicationPackagePath, Hashtable applicationParameters, string imageStoreConnectionString)
        {
            var testRootPath = Path.Combine(Path.GetTempPath(), string.Format(CultureInfo.InvariantCulture, "{0}_{1}", Constants.BaseTestRootFolderName, Stopwatch.GetTimestamp()));

            try
            {
                var absolutePath = this.GetAbsolutePath(applicationPackagePath);
                var applicationName = this.GetPackageName(absolutePath);
                var testRootApplicationPath = Path.Combine(testRootPath, applicationName);
                FabricDirectory.Copy(absolutePath, testRootApplicationPath, true);
                ImageBuilderUtility.RemoveReadOnlyFlag(testRootApplicationPath);

                var fileImageStoreConnectionString =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "{0}{1}",
                        Constants.ImageStoreConnectionFileType,
                        testRootPath.TrimEnd(Path.DirectorySeparatorChar));
                var sourceImageStore = ImageStoreFactoryProxy.CreateImageStore(fileImageStoreConnectionString);

                IImageStore destinationImageStore = null;
                if (string.IsNullOrEmpty(imageStoreConnectionString))
                {
                    destinationImageStore = sourceImageStore;
                }
                else
                {
                    destinationImageStore = this.CreateImageStore(imageStoreConnectionString, testRootPath);
                }

                var imagebuilder = new ImageBuilder(
                    sourceImageStore,
                    destinationImageStore,
                    this.GetFabricFilePath(Constants.ServiceModelSchemaFileName),
                    testRootPath);

                // Set SFVolumeDiskServiceEnabled to true so that client side validation does the right thing
                // if corresponding UseServiceFabricReplicatedStore attribute is set.
                //
                // We always have a final validation at runtime to catch any issues.
                imagebuilder.IsSFVolumeDiskServiceEnabled = true;

                imagebuilder.ValidateApplicationPackage(
                    applicationName,
                    this.GetDictionary(applicationParameters));

                this.WriteObject(true);
            }
            catch (AggregateException exception)
            {
                exception.Handle((ae) =>
                {
                    this.WriteObject(false);
                    this.ThrowTerminatingError(
                        ae,
                        Constants.TestApplicationPackageErrorId,
                        null);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.WriteObject(false);
                this.ThrowTerminatingError(
                    exception,
                    Constants.TestApplicationPackageErrorId,
                    null);
            }
            finally
            {
                FabricDirectory.Delete(testRootPath, true, true);
            }
        }

        protected void RemoveApplicationPackage(string imageStoreConnectionString, string applicationPackagePath)
        {
            try
            {
                if (imageStoreConnectionString.StartsWith(Constants.ImageStoreConnectionFabricType, StringComparison.OrdinalIgnoreCase))
                {
                    var clusterConnection = this.SessionState.PSVariable.GetValue(Constants.ClusterConnectionVariableName, null) as IClusterConnection;
                    if (clusterConnection.SecurityCredentials != null && clusterConnection.SecurityCredentials.CredentialType == CredentialType.Claims)
                    {
                        clusterConnection.FabricClient.ImageStore.DeleteContent(imageStoreConnectionString, applicationPackagePath, this.GetTimeout());
                        this.WriteObject(StringResources.Info_RemoveApplicationPackageSucceeded);
                        return;
                    }
                }

                IImageStore imageStore = this.CreateImageStore(imageStoreConnectionString, Path.GetTempPath());
                imageStore.DeleteContent(applicationPackagePath, this.GetTimeout());
                this.WriteObject(StringResources.Info_RemoveApplicationPackageSucceeded);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.RemoveApplicationPackageErrorId,
                    null);
            }
        }

        protected void RegisterApplicationType(ProvisionApplicationTypeDescriptionBase description)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.ProvisionApplicationAsync(
                    description,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(description.Async 
                    ? StringResources.Info_RegisterApplicationStarted
                    : StringResources.Info_RegisterApplicationSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RegisterApplicationTypeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UnregisterApplicationType(UnprovisionApplicationTypeDescription description)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UnprovisionApplicationAsync(
                    description,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                this.WriteObject(description.Async 
                    ? StringResources.Info_UnregisterApplicationStarted
                    : StringResources.Info_UnregisterApplicationSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UnregisterApplicationTypeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void CreateApplicationInstance(
            Uri applicationName,
            string applicationType,
            string applicationVersion,
            Hashtable applicationParameters,
            long? maximumNodes,
            long? minimumNodes,
            string[] metrics)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var applicationDescription =
                    new ApplicationDescription(applicationName, applicationType, applicationVersion, this.GetNameValueCollection(applicationParameters));

                if (maximumNodes.HasValue)
                {
                    applicationDescription.MaximumNodes = maximumNodes.Value;
                }

                if (minimumNodes.HasValue)
                {
                    applicationDescription.MinimumNodes = minimumNodes.Value;
                }

                if (metrics != null)
                {
                    foreach (var scaleoutMetric in metrics)
                    {
                        applicationDescription.Metrics.Add(this.ParseScaleoutMetric(scaleoutMetric));
                    }
                }

                ApplicationDescription.Validate(applicationDescription);

                clusterConnection.CreateApplicationAsync(
                    applicationDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(applicationDescription));
            }
            catch (ArgumentException exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.CreateApplicationInstanceErrorId,
                    clusterConnection);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.CreateApplicationInstanceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UpdateApplicationInstance(
            Uri applicationName,
            bool removeApplicationCapacity,
            long? minimumNodes,
            long? maximumNodes,
            string[] metrics)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var updateDescription = new ApplicationUpdateDescription(applicationName);

                updateDescription.RemoveApplicationCapacity = removeApplicationCapacity;

                updateDescription.MinimumNodes = minimumNodes;
                updateDescription.MaximumNodes = maximumNodes;

                if (metrics != null)
                {
                    updateDescription.Metrics = new List<ApplicationMetricDescription>();
                    foreach (var scaleoutMetric in metrics)
                    {
                        updateDescription.Metrics.Add(this.ParseScaleoutMetric(scaleoutMetric));
                    }
                }

                ApplicationUpdateDescription.Validate(updateDescription);
                clusterConnection.UpdateApplicationAsync(
                    updateDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(updateDescription));
            }
            catch (ArgumentException exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpdateApplicationInstanceErrorId,
                    clusterConnection);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpdateApplicationInstanceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void RemoveApplicationInstance(DeleteApplicationDescription deleteDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.DeleteApplicationAsync(
                    deleteDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_RemoveApplicationInstanceSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RemoveApplicationInstanceErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UpgradeApplication(ApplicationUpgradeDescription upgradeDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpgradeApplicationAsync(
                    upgradeDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(upgradeDescription), true);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpgradeApplicationErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void UpdateApplicationUpgrade(ApplicationUpgradeUpdateDescription updateDescription)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.UpdateApplicationUpgradeAsync(
                    updateDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(this.FormatOutput(updateDescription), true);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.UpdateApplicationUpgradeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected void RollbackApplicationUpgrade(Uri applicationName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.RollbackApplicationUpgradeAsync(
                    applicationName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.RollbackApplicationUpgradeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected ApplicationUpgradeProgress GetApplicationUpgradeProgress(Uri applicationName)
        {
            var clusterConnection = this.GetClusterConnection();

            ApplicationUpgradeProgress currentProgress = null;
            try
            {
                currentProgress =
                    clusterConnection.GetApplicationUpgradeProgressAsync(
                        applicationName,
                        this.GetTimeout(),
                        this.GetCancellationToken()).Result;
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetApplicationUpgradeProgressErrorId,
                        clusterConnection);
                    return true;
                });
            }

            return currentProgress;
        }

        protected void MoveNextApplicationUpgradeDomain(Uri applicationName, string upgradeDomainName)
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.MoveNextApplicationUpgradeDomainAsync(
                    applicationName,
                    upgradeDomainName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_ResumeApplicationUpgradeSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.ResumeApplicationUpgradeDomainErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.UpgradeApplicationErrorId,
                    null);
            }
        }       

        protected PSObject ToPSObject(ApplicationUpgradeDescription upgradeDescription)
        {
            var itemPSObj = new PSObject(upgradeDescription);

            if (upgradeDescription.ApplicationParameters != null)
            {
                var parametersPropertyPSObj = new PSObject(upgradeDescription.ApplicationParameters);
                parametersPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ApplicationParametersPropertyName,
                        parametersPropertyPSObj));
            }

            if (upgradeDescription.UpgradePolicyDescription == null)
            {
                return itemPSObj;
            }

            var monitoredPolicy = upgradeDescription.UpgradePolicyDescription as MonitoredRollingApplicationUpgradePolicyDescription;
            if (monitoredPolicy != null)
            {
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, monitoredPolicy.Kind));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, monitoredPolicy.ForceRestart));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, monitoredPolicy.UpgradeMode));
                itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, monitoredPolicy.UpgradeReplicaSetCheckTimeout));

                if (monitoredPolicy.MonitoringPolicy != null)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.FailureActionPropertyName, monitoredPolicy.MonitoringPolicy.FailureAction));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckWaitDurationPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckWaitDuration));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckStableDurationPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckStableDuration));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.HealthCheckRetryTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.HealthCheckRetryTimeout));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeDomainTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.UpgradeDomainTimeout));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeTimeoutPropertyName, monitoredPolicy.MonitoringPolicy.UpgradeTimeout));
                }

                if (monitoredPolicy.HealthPolicy != null)
                {
                    this.AddToPSObject(itemPSObj, monitoredPolicy.HealthPolicy);
                }
            }
            else
            {
                var policy = upgradeDescription.UpgradePolicyDescription as RollingUpgradePolicyDescription;
                if (policy != null)
                {
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeKindPropertyName, policy.Kind));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.ForceRestartPropertyName, policy.ForceRestart));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeModePropertyName, policy.UpgradeMode));
                    itemPSObj.Properties.Add(new PSNoteProperty(Constants.UpgradeReplicaSetCheckTimeoutPropertyName, policy.UpgradeReplicaSetCheckTimeout));
                }
            }

            return itemPSObj;
        }

        protected void AddToPSObject(PSObject itemPSObj, ApplicationHealthPolicy healthPolicy)
        {
            if (healthPolicy == null)
            {
                return;
            }

            itemPSObj.Properties.Add(new PSNoteProperty(Constants.ConsiderWarningAsErrorPropertyName, healthPolicy.ConsiderWarningAsError));

            itemPSObj.Properties.Add(
                new PSNoteProperty(Constants.MaxPercentUnhealthyDeployedApplicationsPropertyName, healthPolicy.MaxPercentUnhealthyDeployedApplications));

            if (healthPolicy.DefaultServiceTypeHealthPolicy != null)
            {
                itemPSObj.Properties.Add(
                    new PSNoteProperty(Constants.MaxPercentUnhealthyPartitionsPerServicePropertyName, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyPartitionsPerService));
                itemPSObj.Properties.Add(
                    new PSNoteProperty(Constants.MaxPercentUnhealthyReplicasPerPartitionPropertyName, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyReplicasPerPartition));
                itemPSObj.Properties.Add(
                    new PSNoteProperty(Constants.MaxPercentUnhealthyServicesPropertyName, healthPolicy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices));
            }

            if (healthPolicy.ServiceTypeHealthPolicyMap != null)
            {
                var serviceTypeHealthPolicyMapPropertyPSObj = new PSObject(healthPolicy.ServiceTypeHealthPolicyMap);
                serviceTypeHealthPolicyMapPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));
                itemPSObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ServiceTypeHealthPolicyMapPropertyName,
                        serviceTypeHealthPolicyMapPropertyPSObj));
            }
        }

        protected ServiceTypeHealthPolicy ParseServiceTypeHealthPolicy(string serviceTypeHealthPolicy)
        {
            if (string.IsNullOrEmpty(serviceTypeHealthPolicy))
            {
                return new ServiceTypeHealthPolicy();
            }

            string[] policyFields = serviceTypeHealthPolicy.Split(',', ' ');

            if (policyFields.Length != 3)
            {
                throw new ArgumentException(StringResources.Error_InvalidServiceTypeHealthPolicySpecification);
            }

            return new ServiceTypeHealthPolicy
            {
                MaxPercentUnhealthyPartitionsPerService = byte.Parse(policyFields[0]),
                MaxPercentUnhealthyReplicasPerPartition = byte.Parse(policyFields[1]),
                MaxPercentUnhealthyServices = byte.Parse(policyFields[2])
            };
        }

        private ApplicationMetricDescription ParseScaleoutMetric(string metricStr)
        {
            long reservationCapacity = 0;
            long maximumCapacity = 0;
            long totalCapacity = 0;

            // check for duplicate tags
            bool isReservationCapacitySet = false;
            bool isMaximumCapacitySet = false;
            bool isTotalCapacitySet = false;

            string[] metricFields = metricStr.Split(',', ' ');

            // Metric specified with tags
            // Format is [NodeReservationCapacity:val1][,MaximumNodeCapacity:val2][,TotalApplicationCapacity:val3]
            // Default format is: NodeReservationCapacity,MaximumNodeCapacity,TotalApplicationCapacity
            if (metricStr.LastIndexOf(':') > metricFields[0].Length)
            {
                for (int index = 1; index < metricFields.Length; ++index)
                {
                    string[] metricFieldSplit = metricFields[index].Split(':');

                    if (metricFieldSplit.Length != 2)
                    {
                        throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                    }

                    if (metricFieldSplit[0].CompareTo("NodeReservationCapacity") == 0)
                    {
                        if (isReservationCapacitySet)
                        {
                            throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                        }

                        if (!long.TryParse(metricFieldSplit[1], out reservationCapacity))
                        {
                            throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                        }

                        isReservationCapacitySet = true;
                    }
                    else if (metricFieldSplit[0].CompareTo("MaximumNodeCapacity") == 0)
                    {
                        if (isMaximumCapacitySet)
                        {
                            throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                        }

                        if (!long.TryParse(metricFieldSplit[1], out maximumCapacity))
                        {
                            throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                        }

                        isMaximumCapacitySet = true;
                    }
                    else if (metricFieldSplit[0].CompareTo("TotalApplicationCapacity") == 0)
                    {
                        if (isTotalCapacitySet)
                        {
                            throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                        }

                        if (!long.TryParse(metricFieldSplit[1], out totalCapacity))
                        {
                            throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                        }

                        isTotalCapacitySet = true;
                    }
                    else  
                    {
                        throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                    }
                }
            }
            else 
            {
                if (metricFields.Length != 4)
                {
                    throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                }

                if (!long.TryParse(metricFields[1], out reservationCapacity))
                {
                    throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                }

                if (!long.TryParse(metricFields[2], out maximumCapacity))
                {
                    throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                }

                if (!long.TryParse(metricFields[3], out totalCapacity))
                {
                    throw new ArgumentException(StringResources.Error_InvalidApplicationMetricSpecification);
                }
            }

            return new ApplicationMetricDescription()
            {
                Name = metricFields[0],
                NodeReservationCapacity = reservationCapacity,
                MaximumNodeCapacity = maximumCapacity,
                TotalApplicationCapacity = totalCapacity
            };
        }
    }
}