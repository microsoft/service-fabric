// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Security;
    using System.Text;
    using System.Xml; 

    internal struct UnreliableTransportSpecification
    {
        public string Name;
        public string Specification;        
    }

    internal class UnreliableTransportSettings
    {
        public const string SettingsFileName = "UnreliableTransportSettings.ini";

        public List<UnreliableTransportSpecification> Specification { get; private set; }

        public UnreliableTransportSettings(ClusterSettings clusterSettings)
        {
            Specification = new List<UnreliableTransportSpecification>();
            LoadSettingsFromClusterSettings(clusterSettings);
        }

        public UnreliableTransportSettings(string filePath)
        {
            Specification = new List<UnreliableTransportSpecification>();
            LoadSettingsFromUnreliableTransportSettingsFile(filePath);
        }
        private void LoadSettingsFromClusterSettings(ClusterSettings clusterSettings)
        {
            foreach (var setting in clusterSettings.Settings)
            {
                if (setting.Name == FabricValidatorConstants.SectionNames.UnreliableTransport)
                {
                    foreach (var parameter in setting.Parameter)
                    {
                        UnreliableTransportSpecification specification = new UnreliableTransportSpecification();
                        if (!parameter.Name.Contains('=') && ParseSpecificationFromClusterSettings(parameter.Name, parameter.Value, ref specification) != Constants.ErrorCode_Failure)
                        {                            
                                Specification.Add(specification);
                        }
                        else
                        {
                            DeployerTrace.WriteWarning("Skipping malformed UnreliableTransport line in ClusterManifest File, value = [name = {0} value = {1}]", parameter.Name, parameter.Value);
                        }
                    }

                }
            }
        }
        private void LoadSettingsFromUnreliableTransportSettingsFile(string filePath)
        {
            if (File.Exists(filePath) == true)
            {
                try
                {
                    using (FileStream fileStream = new FileStream(filePath, FileMode.Open, FileAccess.Read, FileShare.Read))
                    {
                        using (StreamReader file = new StreamReader(fileStream))
                        {
                            string specificationString;

                            while ((specificationString = file.ReadLine()) != null)
                            {
                                UnreliableTransportSpecification specification = new UnreliableTransportSpecification();
                                if (Constants.ErrorCode_Success == ParseSpecificationFromINI(specificationString, ref specification))
                                {
                                    Specification.Add(specification);
                                }
                                else
                                {
                                    DeployerTrace.WriteWarning("Skipping malformed line in UnreliableTransportSettings File, value = [{0}]", specificationString);
                                }
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    DeployerTrace.WriteError("Unable to read Unreliable Settings file because {0}", e);
                    throw;
                }
            }
        }
        private int ParseSpecificationFromINI(string specificationLine, ref UnreliableTransportSpecification specification)
        {

            string[] specificationSplit = specificationLine.Split('=');

            // check whether specification is malformed
            if (specificationSplit.Length > 0)
            {
                // behavior name (separated from remaining properties by '=')
                specification.Name = specificationSplit[0];
            }
            else
            {
                return Constants.ErrorCode_Success;
            }

            specification.Specification = (specificationSplit.Length > 1) ? specificationSplit[1] : "";

            return Constants.ErrorCode_Failure;

        }

        private int ParseSpecificationFromClusterSettings(string name, string value, ref UnreliableTransportSpecification specification)
        {

            // behavior name 
            specification.Name = name;
            specification.Specification = value;         

            return Constants.ErrorCode_Success;
        }

        public void WriteUnreliableTransportSettingsFile(string filePath)
        {
            try
            {
                using (FileStream fileStream = new FileStream(filePath, FileMode.Create, FileAccess.Write, FileShare.Read))
                {
                    using (StreamWriter file = new StreamWriter(fileStream))
                    {
                        foreach (var specification in Specification)
                        {
                            file.WriteLine("{0}={1}",
                                specification.Name,
                                specification.Specification);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("Unable to write UnreliableTransportSettings file at {0} because of {1}", filePath, e);
                throw;
            }
        }

        public void Merge(UnreliableTransportSettings unreliableTransportSettings)
        {
            Dictionary<string, UnreliableTransportSpecification> specificationsDictionary = new Dictionary<string, UnreliableTransportSpecification>();

            // populate dictionary with this settings
            foreach (var specification in Specification)
            {
                specificationsDictionary.Add(specification.Name, specification);
            }

            // now we merge the settings with precedence of specifications coming from this
            foreach (var specification in unreliableTransportSettings.Specification)
            {
                if (specificationsDictionary.ContainsKey(specification.Name) == false)
                {
                    specificationsDictionary.Add(specification.Name, specification);
                }
                else
                {
                    specificationsDictionary[specification.Name] = specification;
                    DeployerTrace.WriteWarning("Conflicting Unreliable Transport Behavior when merging named {0}. Replacing with new behavior", specification.Name);
                }
            }

            // removing previous specifications to populate with ones in dictionary
            Specification.Clear();
            Specification.AddRange(specificationsDictionary.Values);
        }
    }    
}