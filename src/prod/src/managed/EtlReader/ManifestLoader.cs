// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Security.Cryptography.X509Certificates;
    using System.Xml;

    public class ManifestLoader
    {
        public ManifestDefinitionDescription LoadManifest(string fileName)
        {
            if (!File.Exists(fileName))
            {
                throw new FileNotFoundException(
                    string.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_ManifestFileNotFound_Formatted, fileName),
                    fileName);
            }

            return this.LoadManifest(File.OpenRead(fileName));
        }

        public ManifestDefinitionDescription LoadManifest(Stream stream)
        { 
            XmlDocument doc = new XmlDocument { XmlResolver = null };
            using (XmlTextReader reader = new XmlTextReader(stream) { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null })
            {
                doc.Load(reader);
            }

            XmlNamespaceManager mgr = new XmlNamespaceManager(doc.NameTable);
            mgr.AddNamespace("asm", "urn:schemas-microsoft-com:asm.v3");
            mgr.AddNamespace("evt", "http://schemas.microsoft.com/win/2004/08/events");

            XmlElement rootNode = doc.DocumentElement;
            if (rootNode == null)
            {
                throw GetException(String.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_MissingElement_Formatted, "root"));
            }

            XmlElement instrumentationNode = rootNode["instrumentation"];
            if (instrumentationNode == null)
            {
                throw GetException(String.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_MissingElement_Formatted, "instrumentation"));
            }

            XmlElement providerElements = instrumentationNode["events"];
            if (providerElements == null)
            {
                throw GetException(String.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_MissingElement_Formatted, "instrumentation/events"));
            }

            XmlElement localizationNode = rootNode["localization"];
            if (localizationNode == null)
            {
                throw GetException(String.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_MissingElement_Formatted, "localization"));
            }

            XmlElement resources = localizationNode["resources"]; // BUGBUG - which locale?
            if (resources == null)
            {
                throw GetException(String.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_MissingElement_Formatted, "localization/resources"));
            }

            XmlElement stringTable = resources["stringTable"];
            if (stringTable == null)
            {
                throw GetException(String.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_MissingElement_Formatted, "localization/resources/stringTable"));
            }

            StringTable strings = new StringTable(stringTable);

            List<ProviderDefinitionDescription> providers = new List<ProviderDefinitionDescription>();
            foreach (XmlElement provider in providerElements.GetElementsByTagName("provider"))
            {
                providers.Add(new ProviderDefinitionDescription(provider, strings));
            }

            return new ManifestDefinitionDescription { Providers = providers };
        }

        private static XmlException GetException(string reason)
        {
            return new XmlException(string.Format(CultureInfo.CurrentCulture, StringResources.ETLReaderError_InvalidManifestDocument_Formatted, reason));
        }
    }
}