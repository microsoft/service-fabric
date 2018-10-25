// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.ImageModel;
    using System.Globalization;
    using System.Linq;
    using System.Xml;

    // Class that wraps service configuration
    internal class ServiceConfig
    {
        internal const string EtwElement = "ETW";
        internal const string ExeHostElement = "ExeHost";

        private const string TraceType = "ServiceConfig";
        private const string ServicePackageElement = "ServicePackage";
        private const string DiagnosticsElement = "Diagnostics";
        private const string ProviderGuidsElement = "ProviderGuids";
        private const string ProviderGuidElement = "ProviderGuid";
        private const string ManifestDataPackagesElement = "ManifestDataPackages";
        private const string ManifestDataPackageElement = "ManifestDataPackage";
        private const string NameAttribute = "Name";
        private const string ValueAttribute = "Value";
        private const string VersionAttribute = "Version";
        private const string PackageSharedAttribute = "IsShared";
        private const string DigestedCodePackageElement = "DigestedCodePackage";
        private const string CodePackageElement = "CodePackage";
        private const string SetupEntryPointElement = "SetupEntryPoint";
        private const string EntryPointElement = "EntryPoint";
        private const string DllHostElement = "DllHost";
        private const string ProgramElement = "Program";
        private const string FwpExeName = "fwp.exe";

        // Run layout specification
        private readonly RunLayoutSpecification runLayout;

        // ETW provider GUIDs
        private readonly List<Guid> etwProviderGuids;

        // ETW manifest paths
        private readonly List<ServiceEtwManifestInfo> etwManifests;

        // Executables
        private readonly HashSet<string> exeNames;

        // Service package file
        private readonly string servicePackageFile;

        // Application instance ID
        private readonly string appInstanceId;

        // Service package name
        private readonly string servicePackageName;

        internal ServiceConfig(string runLayoutRoot, string appInstanceId, string servicePackageName, string serviceRolloutVersion)
        {
            this.etwProviderGuids = new List<Guid>();
            this.etwManifests = new List<ServiceEtwManifestInfo>();
            this.exeNames = new HashSet<string>();
            this.runLayout = RunLayoutSpecification.Create();
            this.appInstanceId = appInstanceId;
            this.servicePackageName = servicePackageName;

            // Get the service manifest
            this.runLayout.SetRoot(runLayoutRoot);
            this.servicePackageFile = this.runLayout.GetServicePackageFile(
                                           this.appInstanceId,
                                           this.servicePackageName,
                                           serviceRolloutVersion);

            // Load the manifest
            XmlDocument doc = new XmlDocument
            {
#if !DotNetCoreClr
                XmlResolver = null
#endif
            };
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
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
                        using (XmlReader reader = XmlReader.Create(this.servicePackageFile, settings))
                        {
                            doc.Load(reader);
                        }
                    });
            }
            catch (Exception e)
            {
                var message = string.Format(
                    "Error occurred while loading manifest {0}",
                    this.servicePackageFile);
                Utility.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    message);
                throw new InvalidOperationException(message, e);
            }

            // Get the ServicePackage element
            XmlElement rootElem = doc.DocumentElement;
            if (rootElem == null)
            {
                var message = string.Format(
                    "Unable to find the root element in {0}.",
                    this.servicePackageFile);
                Utility.TraceSource.WriteError(
                    TraceType,
                    message);
                throw new InvalidOperationException(message);
            }

            // Get the Diagnostics element
            XmlElement diagnosticsElem = rootElem[DiagnosticsElement];

            // Parse the Diagnostics element
            this.ParseDiagnosticsElement(diagnosticsElem);

            // Parse the manifest to figure out which executables are associated
            // with this service
            this.ParseExes(rootElem);
        }

        internal IEnumerable<Guid> EtwProviderGuids
        {
            get
            {
                return this.etwProviderGuids;
            }
        }

        internal List<ServiceEtwManifestInfo> EtwManifests
        {
            get
            {
                return this.etwManifests;
            }
        }

        internal IEnumerable<string> ExeNames
        {
            get
            {
                return this.exeNames;
            }
        }

        internal static HashSet<string> GetChangedSections(ServiceConfig config1, ServiceConfig config2)
        {
            HashSet<string> changedSections = new HashSet<string>();

            // Provider GUIDs present in config1, but not in config2
            IEnumerable<Guid> removedGuids = Enumerable.Except(config1.EtwProviderGuids, config2.EtwProviderGuids);

            // Provider GUIDs present in config2, but not in config1
            IEnumerable<Guid> addedGuids = Enumerable.Except(config2.EtwProviderGuids, config1.EtwProviderGuids);

            // Manifest paths present in config1, but not in config2
            IEnumerable<ServiceEtwManifestInfo> removedPaths = Enumerable.Except(config1.EtwManifests, config2.EtwManifests);

            // Manifest paths present in config2, but not in config1
            IEnumerable<ServiceEtwManifestInfo> addedPaths = Enumerable.Except(config2.EtwManifests, config1.EtwManifests);

            if (addedGuids.Any() ||
                removedGuids.Any() ||
                addedPaths.Any() ||
                removedPaths.Any())
            {
                changedSections.Add(EtwElement);
            }

            if (config1.ExeNames.Except(config2.ExeNames).Any() ||
                config2.ExeNames.Except(config1.ExeNames).Any())
            {
                changedSections.Add(ExeHostElement);
            }

            return changedSections;
        }

        internal HashSet<string> GetDiagnosticSections()
        {
            HashSet<string> diagnosticSections = new HashSet<string>();
            if (this.etwProviderGuids.Any() || this.etwManifests.Any())
            {
                diagnosticSections.Add(EtwElement);
            }

            if (this.exeNames.Any())
            {
                diagnosticSections.Add(ExeHostElement);
            }

            return diagnosticSections;
        }

        private void ParseDiagnosticsElement(XmlElement diagnosticsElem)
        {
            if (diagnosticsElem == null)
            {
                // Diagnostics element not present
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Diagnostics element is not present in manifest {0}",
                    this.servicePackageFile);
                return;
            }

            // Parse the ETW element
            XmlElement etwElement = diagnosticsElem[EtwElement];
            this.ParseEtwElement(etwElement);
        }

        private void ParseEtwElement(XmlElement etwElement)
        {
            if (etwElement == null)
            {
                // ETW element not present
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "ETW element is not present in manifest {0}",
                    this.servicePackageFile);
                return;
            }

            // Parse the provider GUIDs
            XmlElement providerGuidsElement = etwElement[ProviderGuidsElement];
            this.ParseProviderGuids(providerGuidsElement);

            // Parse the manifest data packages
            XmlElement manifestDataPackagesElement = etwElement[ManifestDataPackagesElement];
            this.ParseManifestDataPackages(manifestDataPackagesElement);
        }

        private void ParseProviderGuids(XmlElement providerGuidsElement)
        {
            if (providerGuidsElement == null)
            {
                // Provider GUIDs element not present
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "{0} element is not present in manifest {1}",
                    ProviderGuidsElement,
                    this.servicePackageFile);
                return;
            }

            foreach (XmlNode childNode in providerGuidsElement.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    // We're only interested in elements
                    continue;
                }

                // Parse the provider GUID string
                XmlElement providerGuidElement = childNode as XmlElement;
                string providerGuidElementName = providerGuidElement.Name.Trim();
                if (providerGuidElementName.Equals(ProviderGuidElement, StringComparison.Ordinal))
                {
                    string guidAsString;
                    if (null != providerGuidElement.Attributes[ValueAttribute])
                    {
                        guidAsString = providerGuidElement.Attributes[ValueAttribute].Value.Trim();
                    }
                    else
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Element {0} encountered in manifest {1} does not have attribute {2}.",
                            ProviderGuidElement,
                            this.servicePackageFile,
                            ValueAttribute);
                        continue;
                    }

                    Guid providerGuid;
                    if (false == Guid.TryParse(guidAsString, out providerGuid))
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "Element {0} encountered in manifest {1} cannot be parsed as a GUID.",
                            ProviderGuidElement,
                            this.servicePackageFile);
                        continue;
                    }

                    this.etwProviderGuids.Add(providerGuid);
                }
                else
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Element {0} encountered in manifest {1} instead of the expected element {2}.",
                        providerGuidElementName,
                        this.servicePackageFile,
                        ProviderGuidElement);
                }
            }
        }

        private void ParseManifestDataPackages(XmlElement manifestDataPackagesElement)
        {
            if (manifestDataPackagesElement == null)
            {
                // Manifest data package element not present
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "{0} element is not present in manifest {1}",
                    ManifestDataPackagesElement,
                    this.servicePackageFile);
                return;
            }

            foreach (XmlNode childNode in manifestDataPackagesElement.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    // We're only interested in elements
                    continue;
                }

                // Parse the manifest data package
                XmlElement manifestDataPackageElement = childNode as XmlElement;
                string manifestDataPackageElementName = manifestDataPackageElement.Name.Trim();
                if (manifestDataPackageElementName.Equals(ManifestDataPackageElement, StringComparison.Ordinal))
                {
                    // Get the "Name" attribute
                    XmlAttribute nameAttr = manifestDataPackageElement.Attributes[NameAttribute];
                    if (null == nameAttr)
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "{0} element does not have the required attribute {1} in manifest {2}",
                            ManifestDataPackageElement,
                            NameAttribute,
                            this.servicePackageFile);
                        return;
                    }

                    string name = nameAttr.Value.Trim();

                    // Get the "Version" attribute
                    XmlAttribute versionAttr = manifestDataPackageElement.Attributes[VersionAttribute];
                    if (null == versionAttr)
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "{0} element does not have the required attribute {1} in manifest {2}",
                            ManifestDataPackageElement,
                            VersionAttribute,
                            this.servicePackageFile);
                        return;
                    }

                    string version = versionAttr.Value.Trim();

                    Version versionNumber;
                    if (false == Version.TryParse(version, out versionNumber))
                    {
                        Utility.TraceSource.WriteError(
                            TraceType,
                            "attribute {0} of element {1} in manifest {2} cannot be parsed as a .NET framework Version type",
                            VersionAttribute,
                            ManifestDataPackageElement,
                            this.servicePackageFile);
                        return;
                    }

                    // Get the "IsShared" attribute
                    bool isShared = false;
                    XmlAttribute sharedAttribute = manifestDataPackageElement.Attributes[PackageSharedAttribute];
                    if (null != sharedAttribute)
                    {
                        string shared = sharedAttribute.Value.Trim();

                        if (false == bool.TryParse(shared, out isShared))
                        {
                            Utility.TraceSource.WriteError(
                                TraceType,
                                "attribute {0} of element {1} in manifest {2} cannot be parsed as a boolean value",
                                PackageSharedAttribute,
                                ManifestDataPackageElement,
                                this.servicePackageFile);
                            return;
                        }
                    }

                    string etwManifestPath = this.runLayout.GetDataPackageFolder(
                                                this.appInstanceId,
                                                this.servicePackageName,
                                                name,
                                                version,
                                                isShared);
                    ServiceEtwManifestInfo manifestInfo = new ServiceEtwManifestInfo()
                    {
                        DataPackageName = name,
                        DataPackageVersion = versionNumber,
                        Path = CultureInfo.InvariantCulture.TextInfo.ToLower(etwManifestPath)
                    };
                    this.etwManifests.Add(manifestInfo);
                }
                else
                {
                    Utility.TraceSource.WriteError(
                        TraceType,
                        "Element {0} encountered in manifest {1} instead of the expected element {2}.",
                        manifestDataPackageElementName,
                        this.servicePackageFile,
                        ManifestDataPackageElement);
                }
            }
        }

        private void ParseExes(XmlElement rootElem)
        {
            foreach (XmlNode childNode in rootElem.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    // We're only interested in elements
                    continue;
                }

                XmlElement childElement = childNode as XmlElement;
                string childElementName = childElement.Name.Trim();
                if (false == childElementName.Equals(DigestedCodePackageElement, StringComparison.Ordinal))
                {
                    continue;
                }

                XmlElement codePackageElement = childElement[CodePackageElement];

                // Check if there is a setup entry point
                XmlElement setupEntryPointElement = codePackageElement[SetupEntryPointElement];
                if (null != setupEntryPointElement)
                {
                    this.ParseExeHost(setupEntryPointElement);
                }

                // Parse the entry point
                XmlElement entryPointElement = codePackageElement[EntryPointElement];

                // Check if the entry point is ExeHost-based or DllHost-based
                XmlElement dllHostElement = entryPointElement[DllHostElement];
                if (null != dllHostElement)
                {
                    this.exeNames.Add(FwpExeName);
                }
                else
                {
                    this.ParseExeHost(entryPointElement);
                }
            }
        }

        private void ParseExeHost(XmlElement parentElement)
        {
            XmlElement exeHostElement = parentElement[ExeHostElement];
            if (exeHostElement == null)
            {
                return;
            }

            XmlElement programElement = exeHostElement[ProgramElement];
            if (programElement == null)
            {
                return;
            }

            string exeName = programElement.InnerText.Trim();
            this.exeNames.Add(exeName);
        }
    }
}