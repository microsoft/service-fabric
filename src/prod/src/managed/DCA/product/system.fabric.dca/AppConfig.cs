// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.ImageModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Xml;

    // Class that wraps application configuration
    internal class AppConfig
    {
        internal const string FolderSourceElement = "FolderSource";
        internal const string CrashDumpSourceElement = "CrashDumpSource";
        internal const string EtwSourceElement = "ETWSource";
        internal const string PerformanceCounterSourceElement = "PerformanceCounterSource";
        internal const string LocalStoreElement = "LocalStore";
        internal const string FileStoreElement = "FileStore";
        internal const string AzureBlobElement = "AzureBlob";
        internal const string AzureTableElement = "AzureTable";

        internal const string AccessInformationElement = "AccessInformation";
        internal const string DomainUserElement = "DomainUser";
        internal const string ManagedServiceAccountElement = "ManagedServiceAccount";
        internal const string AccountNameElement = "AccountName";
        internal const string AccountPasswordElement = "AccountPassword";
        internal const string AccountPasswordIsEncryptedElement = "AccountPasswordIsEncrypted";
        internal const string AccountTypeElement = "AccountType";

        private const string TraceType = "AppConfig";
        private const string DigestedEnvironmentElement = "DigestedEnvironment";
        private const string DiagnosticsElement = "Diagnostics";
        private const string DestinationsElement = "Destinations";
        private const string ParametersElement = "Parameters";
        private const string ParameterElement = "Parameter";
        private const string NameAttribute = "Name";
        private const string ValueAttribute = "Value";
        private const string ApplicationTypeNameAttribute = "ApplicationTypeName";

        // Map to figure out the producer type
        private static readonly Dictionary<string, string> ProducerTypeMap =
            new Dictionary<string, string>()
            {
                { FolderSourceElement, StandardPluginTypes.FolderProducer },
                { CrashDumpSourceElement, StandardPluginTypes.FolderProducer },
                { EtwSourceElement, StandardPluginTypes.EtlFileProducer },
            };

        // Map to figure out the consumer type based on source and destination
        private static readonly Dictionary<string, Dictionary<string, string>> ConsumerTypeMap =
            new Dictionary<string, Dictionary<string, string>>()
            {
                {
                    FolderSourceElement,
                    new Dictionary<string, string>()
                    {
                        { LocalStoreElement, StandardPluginTypes.FileShareFolderUploader },
                        { FileStoreElement, StandardPluginTypes.FileShareFolderUploader },
                        { AzureBlobElement, StandardPluginTypes.AzureBlobFolderUploader },
                    }
                },
                {
                    CrashDumpSourceElement,
                    new Dictionary<string, string>()
                    {
                        { LocalStoreElement, StandardPluginTypes.FileShareFolderUploader },
                        { FileStoreElement, StandardPluginTypes.FileShareFolderUploader },
                        { AzureBlobElement, StandardPluginTypes.AzureBlobFolderUploader },
                    }
                },
                {
                    EtwSourceElement,
                    new Dictionary<string, string>()
                    {
                        { LocalStoreElement, StandardPluginTypes.FileShareEtwCsvUploader },
                        { FileStoreElement, StandardPluginTypes.FileShareEtwCsvUploader },
                        { AzureBlobElement, StandardPluginTypes.AzureBlobEtwCsvUploader },
                        { AzureTableElement, StandardPluginTypes.AzureTableEtwEventUploader },
                    }
                },
                {
                    PerformanceCounterSourceElement,
                    new Dictionary<string, string>()
                    {
                        { LocalStoreElement, StandardPluginTypes.FileShareFolderUploader },
                        { FileStoreElement, StandardPluginTypes.FileShareFolderUploader },
                        { AzureBlobElement, StandardPluginTypes.AzureBlobFolderUploader },
                    }
                },
            };

        // Diagnostic settings in the application manifest
        private readonly Dictionary<string, Dictionary<string, string>> configValues;

        // List of producer names
        private readonly List<string> producerNames;

        // List of consumer names
        private readonly List<string> consumerNames;

        // Run layout specification
        private readonly RunLayoutSpecification runLayout;

        // Name of the node on which the application is located
        private readonly string nodeName;

        // Application instance ID
        private readonly string appInstanceId;

        internal AppConfig(string nodeName, string runLayoutRoot, string appInstanceId, string rolloutVersion)
        {
            this.configValues = new Dictionary<string, Dictionary<string, string>>();
            this.producerNames = new List<string>();
            this.consumerNames = new List<string>();
            this.runLayout = RunLayoutSpecification.Create();

            this.nodeName = nodeName;
            this.appInstanceId = appInstanceId;

            // Get the application manifest
            this.runLayout.SetRoot(runLayoutRoot);
            this.ApplicationManifest = this.runLayout.GetApplicationPackageFile(
                                           this.appInstanceId,
                                           rolloutVersion);

            // Load the manifest
            bool manifestFound = false;
#if DotNetCoreClr
            XmlDocument doc = new XmlDocument();
#else
            XmlDocument doc = new XmlDocument { XmlResolver = null };
#endif
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            XmlReaderSettings settings = new XmlReaderSettings()
                            {
#if DotNetCoreClr
                                DtdProcessing = DtdProcessing.Prohibit
#else
                                DtdProcessing = DtdProcessing.Prohibit,
                                XmlResolver = null
#endif
                            };
                            using (XmlReader reader = XmlReader.Create(this.ApplicationManifest, settings))
                            {
                                doc.Load(reader);
                            }

                            manifestFound = true;
                        }
                        catch (Exception e)
                        {
                            DirectoryNotFoundException dnfe = e as DirectoryNotFoundException;
                            FileNotFoundException fnfe = e as FileNotFoundException;
                            if ((null != dnfe) || (null != fnfe))
                            {
                                Utility.TraceSource.WriteWarning(
                                    TraceType,
                                    "Manifest {0} was not found. This could be because the application has been deleted, but we haven't yet processed the event that notifies us about the deletion.",
                                    this.ApplicationManifest);
                            }
                            else
                            {
                                throw;
                            }
                        }
                    });
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Error occurred while loading manifest {0}",
                    this.ApplicationManifest);
                Utility.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    message);
                throw new InvalidOperationException(message, e);
            }

            if (false == manifestFound)
            {
                var message = string.Format(
                    "Error occurred while loading manifest {0}",
                    this.ApplicationManifest);
                Utility.TraceSource.WriteError(
                    TraceType,
                    message);
                throw new InvalidOperationException(message);
            }

            // Get the ApplicationManifest element
            XmlElement rootElem = doc.DocumentElement;
            if (rootElem == null)
            {
                var message = string.Format(
                    "Unable to find the root element in {0}.",
                    this.ApplicationManifest);
                Utility.TraceSource.WriteError(
                    TraceType,
                    message);
                throw new InvalidOperationException(message);
            }

            XmlAttribute applicationTypeAttr = rootElem.Attributes[ApplicationTypeNameAttribute];
            if (null == applicationTypeAttr)
            {
                var message = string.Format(
                    "Unable to find the {0} attribute in root in {1}.",
                    ApplicationTypeNameAttribute,
                    this.ApplicationManifest);
                Utility.TraceSource.WriteError(
                    TraceType,
                    message);
                throw new InvalidOperationException(message);
            }

            this.ApplicationType = applicationTypeAttr.Value.Trim();

            // Get the DigestedEnvironment element
            XmlElement digestedEnvironmentElem = rootElem[DigestedEnvironmentElement];

            // Get the Diagnostics element
            XmlElement diagnosticsElem = digestedEnvironmentElem[DiagnosticsElement];

            // Parse the diagnostics element
            this.ParseDiagnosticsElement(diagnosticsElem);

            // Add the producers and consumers to the config dictionary
            Dictionary<string, string> diagnosticsSection = new Dictionary<string, string>();
            diagnosticsSection[ConfigReader.ProducerInstancesParamName] = string.Join(",", this.producerNames);
            diagnosticsSection[ConfigReader.ConsumerInstancesParamName] = string.Join(",", this.consumerNames);
            this.configValues[ConfigReader.DiagnosticsSectionName] = diagnosticsSection;
        }

        private delegate bool PluginValueCustomProcessor(Dictionary<string, string> values, XmlElement pluginElement);

        // Application type
        internal string ApplicationType { get; private set; }

        // Full path to application manifest
        internal string ApplicationManifest { get; private set; }

        internal static HashSet<string> GetChangedSections(AppConfig config1, AppConfig config2)
        {
            HashSet<string> changedSections = new HashSet<string>();

            // Sections present in config1, but not in config2
            IEnumerable<string> removedSections = Enumerable.Except(config1.configValues.Keys, config2.configValues.Keys);
            foreach (string removedSection in removedSections)
            {
                changedSections.Add(removedSection);
            }

            // Sections present in config2, but not in config1
            IEnumerable<string> addedSections = Enumerable.Except(config2.configValues.Keys, config1.configValues.Keys);
            foreach (string addedSection in addedSections)
            {
                changedSections.Add(addedSection);
            }

            // Sections present in both config1 and config2
            IEnumerable<string> commonSections = Enumerable.Intersect(config1.configValues.Keys, config2.configValues.Keys);
            foreach (string commonSection in commonSections)
            {
                // Values present in common section of config1, but not config2
                IEnumerable<string> removedValues = Enumerable.Except(config1.configValues[commonSection].Keys, config2.configValues[commonSection].Keys);
                if (0 != removedValues.Count())
                {
                    changedSections.Add(commonSection);
                    continue;
                }

                // Values present in common section of config2, but not config1
                IEnumerable<string> addedValues = Enumerable.Except(config2.configValues[commonSection].Keys, config1.configValues[commonSection].Keys);
                if (0 != addedValues.Count())
                {
                    changedSections.Add(commonSection);
                    continue;
                }

                // Values present in common section of both config1 and config2
                IEnumerable<string> commonValues = Enumerable.Intersect(config1.configValues[commonSection].Keys, config2.configValues[commonSection].Keys);
                foreach (string commonValue in commonValues)
                {
                    if (false == config1.configValues[commonSection][commonValue].Equals(
                                     config2.configValues[commonSection][commonValue]))
                    {
                        // Value is not the same in config1 and config2
                        changedSections.Add(commonSection);
                        break;
                    }
                }
            }

            return changedSections;
        }

        internal T GetUnencryptedConfigValue<T>(string sectionName, string valueName, T defaultValue)
        {
            if (this.configValues.ContainsKey(sectionName))
            {
                if (this.configValues[sectionName].ContainsKey(valueName))
                {
                    string valueAsString = this.configValues[sectionName][valueName];
                    return (T)Convert.ChangeType(valueAsString, typeof(T), CultureInfo.InvariantCulture);
                }
            }

            return defaultValue;
        }

        internal string ReadString(string sectionName, string valueName, out bool isEncrypted)
        {
            isEncrypted = false;
            if (this.configValues.ContainsKey(sectionName))
            {
                if (this.configValues[sectionName].ContainsKey(valueName))
                {
                    return this.configValues[sectionName][valueName];
                }
            }

            return string.Empty;
        }

        internal string GetApplicationLogFolder()
        {
            return this.runLayout.GetApplicationLogFolder(this.appInstanceId);
        }

#if !DotNetCoreClr
        internal string GetApplicationEtwTraceFolder()
        {
            return AppEtwTraceFolders.GetTraceFolderForNode(this.nodeName);
        }
#endif

        internal IEnumerable<string> GetConsumersForProducer(string producerInstance)
        {
            List<string> consumers = new List<string>();
            foreach (string consumer in this.consumerNames)
            {
                if (this.configValues[consumer][ConfigReader.ProducerInstanceParamName].Equals(
                        producerInstance, StringComparison.Ordinal))
                {
                    consumers.Add(consumer);
                }
            }

            return consumers;
        }

        private void ParseDiagnosticsElement(XmlElement diagnosticsElem)
        {
            if (diagnosticsElem == null)
            {
                // Diagnostic data collection not enabled for this
                // application instance
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Diagnostic data collection is not enabled in manifest {0}",
                    this.ApplicationManifest);
                return;
            }

            // Go through each child element
            int folderSourceCount = 0;
            int etwSourceCount = 0;
            int crashDumpSourceCount = 0;
            foreach (XmlNode childNode in diagnosticsElem.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    // We're only interested in elements
                    continue;
                }

                // Parse the child element
                XmlElement sourceElem = childNode as XmlElement;
                string sourceElementName = sourceElem.Name.Trim();
                string producerType = this.GetProducerType(sourceElementName);
                if (null != producerType)
                {
                    int producerId;
                    if (sourceElementName.Equals(FolderSourceElement, StringComparison.Ordinal))
                    {
                        producerId = folderSourceCount++;
                    }
                    else if (sourceElementName.Equals(EtwSourceElement, StringComparison.Ordinal))
                    {
                        producerId = etwSourceCount++;
                    }
                    else if (sourceElementName.Equals(CrashDumpSourceElement, StringComparison.Ordinal))
                    {
                        producerId = crashDumpSourceCount++;
                    }
                    else
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Child element {0} of element {1} in manifest {2} was not recognized.",
                            sourceElementName,
                            DiagnosticsElement,
                            this.ApplicationManifest);
                        return;
                    }

                    this.ParseSourceInfo(sourceElem, sourceElementName, producerId, producerType);
                }
            }

            Debug.Assert(folderSourceCount <= 1, "Folder source can be specified only once per application.");
            Debug.Assert(etwSourceCount <= 1, "Etw source can be specified only once per application.");
            Debug.Assert(crashDumpSourceCount <= 1, "Crash dump source can only be specified once per application.");
        }

        private string GetProducerType(string sourceElementName)
        {
            string producerType = null;
            if (ProducerTypeMap.ContainsKey(sourceElementName))
            {
                producerType = ProducerTypeMap[sourceElementName];
            }
            else
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Application manifest {0} contains unsupported source {1}.",
                    this.ApplicationManifest,
                    sourceElementName);
            }

            return producerType;
        }

        private void ParseSourceInfo(XmlElement sourceElem, string sourceElementName, int id, string producerType)
        {
            // Compute the section name
            string sectionName = string.Concat(sourceElementName, id.ToString(CultureInfo.InvariantCulture));

            // Add this section to the list of producer sections
            this.producerNames.Add(sectionName);

            // Create an entry for this section in our configuration dictionary
            Dictionary<string, string> values = new Dictionary<string, string>();
            values[ConfigReader.ProducerTypeParamName] = producerType;
            this.AddPluginValues(values, sourceElem, producerType);
            this.configValues[sectionName] = values;

            // Get the Destinations element
            XmlElement destinationsElem = sourceElem[DestinationsElement];

            // Parse each of the destinations
            this.ParseDestinations(destinationsElem, sourceElementName, sectionName);
        }

        private void ParseDestinations(XmlElement destinationsElem, string sourceElementName, string sourceSectionName)
        {
            if (null == destinationsElem)
            {
                return;
            }

            // Go through each destination
            int localStoreCount = 0;
            int fileStoreCount = 0;
            int azureBlobCount = 0;
            int azureTableCount = 0;
            foreach (XmlNode childNode in destinationsElem.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    // We're only interested in elements
                    continue;
                }

                // Parse the destination
                XmlElement destinationElem = childNode as XmlElement;
                string destinationElementName = destinationElem.Name.Trim();
                var consumerType = this.GetConsumerType(sourceElementName, destinationElementName);
                if (null != consumerType)
                {
                    int consumerId;
                    if (destinationElementName.Equals(LocalStoreElement, StringComparison.Ordinal))
                    {
                        consumerId = localStoreCount++;
                    }
                    else if (destinationElementName.Equals(FileStoreElement, StringComparison.Ordinal))
                    {
                        consumerId = fileStoreCount++;
                    }
                    else if (destinationElementName.Equals(AzureBlobElement, StringComparison.Ordinal))
                    {
                        consumerId = azureBlobCount++;
                    }
                    else if (destinationElementName.Equals(AzureTableElement, StringComparison.Ordinal))
                    {
                        consumerId = azureTableCount++;
                    }
                    else
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Destination element {0} of source {1} in manifest {2} was not recognized.",
                            destinationElementName,
                            sourceElementName,
                            this.ApplicationManifest);
                        return;
                    }

                    this.SaveDestinationInfo(destinationElem, destinationElementName, sourceSectionName, consumerId, consumerType);
                }
            }
        }

        private string GetConsumerType(string sourceElementName, string destinationElementName)
        {
            string consumerType = null;
            if (ConsumerTypeMap.ContainsKey(sourceElementName) &&
                ConsumerTypeMap[sourceElementName].ContainsKey(destinationElementName))
            {
                consumerType = ConsumerTypeMap[sourceElementName][destinationElementName];
            }
            else
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Application manifest {0} contains unsupported destination {1} for source {2}.",
                    this.ApplicationManifest,
                    destinationElementName,
                    sourceElementName);
            }

            return consumerType;
        }

        private void SaveDestinationInfo(XmlElement destinationElem, string destinationElementName, string sourceSectionName, int id, string consumerType)
        {
            // Compute the section name
            string sectionName = string.Concat(sourceSectionName, destinationElementName, id.ToString(CultureInfo.InvariantCulture));

            // Add this section to the list of consumer sections
            this.consumerNames.Add(sectionName);

            // Create an entry for this section in our configuration dictionary
            Dictionary<string, string> values = new Dictionary<string, string>();
            values[ConfigReader.ProducerInstanceParamName] = sourceSectionName;
            values[ConfigReader.ConsumerTypeParamName] = consumerType;
            this.AddPluginValues(values, destinationElem, consumerType);
            this.configValues[sectionName] = values;
        }

        private void AddPluginValues(Dictionary<string, string> values, XmlElement sectionElement, string pluginType)
        {
            foreach (XmlAttribute attr in sectionElement.Attributes)
            {
                string name = attr.Name.Trim();
                string value = attr.Value.Trim();
                values[name] = value;
            }

            XmlElement parametersElement = null;
            foreach (XmlNode childNode in sectionElement.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    // We're only interested in elements
                    continue;
                }

                XmlElement childElement = childNode as XmlElement;
                string childElementName = childElement.Name.Trim();
                if (childElementName.Equals(ParametersElement, StringComparison.Ordinal))
                {
                    // This is a parameters element. Save a reference to it. We'll try
                    // to read its child elements later.
                    parametersElement = childElement;
                    break;
                }
            }

            if (null != parametersElement)
            {
                foreach (XmlNode childNode in parametersElement.ChildNodes)
                {
                    if (childNode.NodeType != XmlNodeType.Element)
                    {
                        // We're only interested in elements
                        continue;
                    }

                    XmlElement childElement = childNode as XmlElement;
                    string childElementName = childElement.Name.Trim();
                    if (childElementName.Equals(ParameterElement, StringComparison.Ordinal))
                    {
                        // Get the "Name" attribute
                        XmlAttribute nameAttr = childElement.Attributes[NameAttribute];
                        if (null != nameAttr)
                        {
                            string name = nameAttr.Value.Trim();

                            // Get the "Value" attribute
                            XmlAttribute valueAttr = childElement.Attributes[ValueAttribute];
                            if (null != valueAttr)
                            {
                                values[name] = valueAttr.Value.Trim();
                            }
                        }
                    }
                }
            }
        }
    }
}