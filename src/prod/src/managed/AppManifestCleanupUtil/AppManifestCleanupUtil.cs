// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AppManifestCleanupUtil
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Fabric.Management.ServiceModel;

    class AppManifestCleanupUtil
    {
        private const string GeneratedServiceTypeExtensionName = "__GeneratedServiceType__";
        private const string GeneratedDefaultServiceName = "DefaultService";
        private const string GeneratedNamesAttributeName = "Name";
        private List<string> validServiceManifestRefs = new List<string>();
        private List<string> validServiceTypeNames = new List<string>();
        private string existingApplicationManifestContents;
        private ApplicationManifestType appManifestType;
        private const string ParameterNameFormat = "{0}_";

        /// <summary>
        /// Mapping of GeneratedId and DefaultServiceName from ServiceManifest.
        /// GeneratedId is stored as key and DefaultServiceName is stored as Value.
        /// </summary>
        private Dictionary<string,string> GeneratedIdDefaultServiceMapping = new Dictionary<string, string>();

        /// <summary>
        /// Removes unused ServiceManifestRefs from Application Manifest, removes extra parameter values from applciation manifest and appinstance Definition files.
        /// </summary>
        /// <param name="appManifestPath">Application Manifest path to cleanup.</param>
        /// <param name="serviceManifestPaths">List of valid service manifest paths. All other ServiceManifestRefs not found in these ServiceManifests will be removed from ApplicationManifest.</param>
        /// <param name="appParamFilePaths">List of App parameter files used by VS. eg. In VS these are Local.xml & Cloud.xml for Local or Cloud deployment.</param>
        public void CleanUp(string appManifestPath, List<string> serviceManifestPaths, List<string> appParamFilePaths)
        {
            if (!File.Exists(appManifestPath))
            {
                return;
            }

            if (serviceManifestPaths.Any(serviceManifestPath => !File.Exists(serviceManifestPath)))
            {
                return;
            }

            try
            {
                this.LoadExistingAppManifest(appManifestPath);
            }
            catch (InvalidOperationException ex)
            {
                Console.Error.WriteLine("Error while deserializing {0}, please verify thats its a valid ServiceFabric ApplciationManifest. {1}", appManifestPath, ex.InnerException.Message);
                Environment.Exit(ex.HResult);
            }

            foreach (string serviceManifestPath in serviceManifestPaths)
            {
                try
                {
                    this.PopulateServiceManifestInfo(serviceManifestPath);
                }
                catch (InvalidOperationException ex)
                {
                    Console.Error.WriteLine("Error while deserializing {0}, please verify thats its a valid ServiceFabric ServiceManifest. {1}", serviceManifestPath, ex.InnerException.Message);
                    Environment.Exit(ex.HResult);
                }
            }

            this.RemoveExtraServiceManifestImports();
            this.RemoveExtraDefaultServicesAndParameters();
            this.SaveApplicationManifestIfNeeded(appManifestPath);

            if (appParamFilePaths != null)
            {
                foreach (var appParamFilePath in appParamFilePaths)
                {
                    try
                    {
                        this.RemoveExtraParametersFromParamFile(appParamFilePath);
                    }
                    catch (InvalidOperationException ex)
                    {
                        Console.Error.WriteLine("Error while deserializing {0}, please verify thats its a valid applciation parameter file. {1}", appParamFilePath, ex.InnerException.Message);
                        Environment.Exit(ex.HResult);
                    }
                }
            }
        }

        private void PopulateServiceManifestInfo(string serviceManifestPath)
        {
            var serviceManifestContents = Utility.LoadContents(serviceManifestPath).Trim();
            var serviceManifestType = XmlSerializationUtility.Deserialize<ServiceManifestType>(serviceManifestContents);
            this.validServiceManifestRefs.Add(serviceManifestType.Name);

            foreach (ServiceTypeType serviceType in serviceManifestType.ServiceTypes)
            {
                this.validServiceTypeNames.Add(serviceType.ServiceTypeName);

                // Load GeneratedId and Default Service from extension.
                if (serviceType.Extensions == null)
                {
                    continue;
                }

                try
                {
                    var extension =
                        serviceType.Extensions.First(x => x.Name.Equals(GeneratedServiceTypeExtensionName));
                    var xmlEnumerator = extension.Any.ChildNodes.GetEnumerator();

                    if (extension.GeneratedId != null)
                    {
                        while (xmlEnumerator.MoveNext())
                        {
                            var xml = xmlEnumerator.Current as XmlElement;

                            if (xml != null && xml.Name.Equals(GeneratedDefaultServiceName))
                            {
                                this.GeneratedIdDefaultServiceMapping.Add(
                                    extension.GeneratedId,
                                    xml.GetAttribute(GeneratedNamesAttributeName));
                                break;
                            }
                        }
                    }
                }
                catch (InvalidOperationException)
                {
                    // no valid extension was found, its ok.
                }
            }
        }

        private void LoadExistingAppManifest(string appManifestPath)
        {
            this.existingApplicationManifestContents = Utility.LoadContents(appManifestPath).Trim();
            this.appManifestType = XmlSerializationUtility.Deserialize<ApplicationManifestType>(this.existingApplicationManifestContents);
        }

        private void RemoveExtraServiceManifestImports()
        {
            // Remove ServiceManifestImports from ApplicationManifest if they were not found in ServiceManifests.
            if (this.appManifestType.ServiceManifestImport != null)
            {
                var serviceManifestImports = this.appManifestType.ServiceManifestImport.ToList<ApplicationManifestTypeServiceManifestImport>();
                serviceManifestImports.RemoveAll(a => !validServiceManifestRefs.Contains(a.ServiceManifestRef.ServiceManifestName));

                // update the serviceManifestImports in ApplicationManifest
                this.appManifestType.ServiceManifestImport = serviceManifestImports.ToArray();
            }
        }

        private void RemoveExtraDefaultServicesAndParameters()
        {
            // Remove DefaultServices from AppManifest if they are not found in ServiceManifests.
            // For each Default Service removed, remove default parameter values for it defined in Parameters section.
            if (this.appManifestType.DefaultServices == null)
            {
                return;
            }

            if (this.appManifestType.DefaultServices.Items != null &&
                this.appManifestType.DefaultServices.Items.Length > 0)
            {
                var defaultServicesList = this.appManifestType.DefaultServices.Items.ToList<object>();
                var servicesToRemove = new List<string>();

                foreach (var service in this.appManifestType.DefaultServices.Items.OfType<DefaultServicesTypeService>())
                {
                    if (!this.validServiceTypeNames.Contains(service.Item.ServiceTypeName))
                    {
                        servicesToRemove.Add(service.Name);
                    }

                    // Remove the service which doesn't have a matching DefaultServiceName
                    // in service manifest associated with the GeneratedIdRef
                    if (service.GeneratedIdRef != null)
                    {
                        string defaultServiceName = null;
                        var found = this.GeneratedIdDefaultServiceMapping.TryGetValue(service.GeneratedIdRef,
                            out defaultServiceName);

                        if (found)
                        {
                            if (!defaultServiceName.Equals(service.Name, StringComparison.OrdinalIgnoreCase))
                            {
                                servicesToRemove.Add(service.Name);
                            }
                        }
                        else
                        {
                            servicesToRemove.Add(service.Name);
                        }
                    }
                }

                defaultServicesList.RemoveAll(
                    item => servicesToRemove.Contains(((DefaultServicesTypeService) item).Name));

                // update the defaultServices in ApplicationManifest
                this.appManifestType.DefaultServices.Items = defaultServicesList.ToArray();

                // Remove parameters for removed services.
                if (this.appManifestType.Parameters != null && this.appManifestType.Parameters.Length > 0)
                {
                    var parameters = this.appManifestType.Parameters.ToList();
                    parameters.RemoveAll(item =>
                        servicesToRemove.Any(
                            serviceName => item.Name.StartsWith(string.Format(ParameterNameFormat, serviceName))));

                    // update parameter list
                    this.appManifestType.Parameters = parameters.ToArray();
                }
            }
        }

        private void SaveApplicationManifestIfNeeded(string appManifestPath)
        {
            string newContent = XmlSerializationUtility.InsertXmlComments(this.existingApplicationManifestContents, this.appManifestType);
            Utility.WriteIfNeeded(appManifestPath, this.existingApplicationManifestContents, newContent);
        }

        private void RemoveExtraParametersFromParamFile(string appParamFilePath)
        {
            var appParamFileContents = Utility.LoadContents(appParamFilePath).Trim();

            var appInstanceDefinition = XmlSerializationUtility.Deserialize<AppInstanceDefinitionType>(appParamFileContents);

            // Remove all extra parameters from app param file which are not in ApplicationManifest after cleanup.
            if (appInstanceDefinition.Parameters != null)
            {
                var allParams = appInstanceDefinition.Parameters.ToList<AppInstanceDefinitionTypeParameter>();

                var validParamNames = new List<string>();
                if (this.appManifestType.Parameters != null)
                {
                    validParamNames = this.appManifestType.Parameters.Select(x => x.Name).ToList();
                }

                allParams.RemoveAll(x => !validParamNames.Contains(x.Name));
                appInstanceDefinition.Parameters = allParams.ToArray();

                string newContent = XmlSerializationUtility.InsertXmlComments(appParamFileContents, appInstanceDefinition);
                Utility.WriteIfNeeded(appParamFilePath, appParamFileContents, newContent);
            }
        }
    }
}