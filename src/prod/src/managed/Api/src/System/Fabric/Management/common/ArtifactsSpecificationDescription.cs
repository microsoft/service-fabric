// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Common
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Management.ImageBuilder;
    using System.IO;
    using System.Linq;
    using System.Reflection;

    internal class ArtifactsSpecificationDescription
    {
        private static readonly string TraceType = "ArtifactsSpecificationDescription";

#if DotNetCoreClr
        private static readonly string ArtifactsSpecification = "System.Fabric.Management.ArtifactsSpecification.csv";
#else
        private static readonly string ArtifactsSpecification = "ArtifactsSpecification.csv";
#endif

        private static Collection<Entry> entries;

        public static IEnumerable<Entry> Entries 
        { 
            get 
            {
                return entries;
            }            
        }

        public enum SigningCertificateType { None, Driver, Native, Managed }

        public class Entry
        {    
            public Entry(
                string feature,
                string guid,
                string fileName,
                string source,
                string destination,
                bool includeInDrop,
                bool includeInManifest,
                bool inGAC,                
                SigningCertificateType signingCertificateType,
                bool installAsNetworkService,
                string displayName,
                string description)
            {
                this.Feature = feature;
                this.Guid = guid;
                this.FileName = fileName;
                this.Source = source;
                this.Description = description;
                this.IncludeInDrop = includeInDrop;
                this.IncludeInManifest = includeInManifest;
                this.InGAC = inGAC;
                this.InstallAsNetworkService = installAsNetworkService;
                this.SigningCertificateType = signingCertificateType;
                this.DisplayName = displayName;
                this.Description = description;
            }

            public string Feature { get; private set; }

            public string Guid { get; private set; }

            public string FileName { get; private set; }

            public string Source { get; private set; }

            public string Destination { get; private set; }

            public bool IncludeInDrop { get; private set; }

            public bool IncludeInManifest { get; private set; }

            public bool InGAC { get; private set; }

            public bool InstallAsNetworkService { get; private set; }

            public SigningCertificateType SigningCertificateType { get; private set; }

            public string DisplayName { get; private set; }

            public string Description { get; private set; }
        }

        static ArtifactsSpecificationDescription()
        {
            entries = new Collection<Entry>();
            try
            {
                Assembly currentAssembly = typeof(ArtifactsSpecificationDescription).GetTypeInfo().Assembly;
                var resourceStream = currentAssembly.GetManifestResourceStream(ArtifactsSpecification);
                TextReader reader = new StreamReader(resourceStream);
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    if (line.StartsWith("#", StringComparison.Ordinal))
                    {
                        continue;
                    }

                    string[] artifactInfo = line.Split(',');

                    ReleaseAssert.AssertIf(artifactInfo == null || artifactInfo.Count() != 12, "Error when reading ArtifactsSpecification.csv file. Line: {0}", line);

                    Entry entry = new Entry(
                        artifactInfo[0].Trim(),
                        artifactInfo[1].Trim(),
                        artifactInfo[2].Trim(),
                        artifactInfo[3].Trim(),
                        artifactInfo[4].Trim(),
                        ImageBuilderUtility.Equals(artifactInfo[5].Trim(), "TRUE"),
                        ImageBuilderUtility.Equals(artifactInfo[6].Trim(), "TRUE"),
                        ImageBuilderUtility.Equals(artifactInfo[7].Trim(), "TRUE"),
                        (SigningCertificateType)Enum.Parse(typeof(SigningCertificateType), artifactInfo[8].Trim(), true),
                        !ImageBuilderUtility.Equals(artifactInfo[9].Trim(), "no"),
                        artifactInfo[10].Trim(),
                        artifactInfo[11].Trim());

                    entries.Add(entry);
                }
            }
            catch (IOException exception)
            {
                ImageBuilder.TraceSource.WriteError(
                    TraceType,
                    "There was an error when reading the ArtifactsSpecification.csv file: {0}",
                    exception);
                throw;
            }
        }
    }
}