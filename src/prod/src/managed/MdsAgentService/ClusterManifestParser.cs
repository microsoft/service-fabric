// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System.IO;
    using System.Xml;
    using System.Xml.XPath;

    internal static class ClusterManifestParser
    {
        public static string ParseMdsUploaderPath(string manifest)
        {
            var settings = new XmlReaderSettings { XmlResolver = null };
            var doc = new XmlDocument { XmlResolver = null };

            using (var stringReader = new StringReader(manifest))
            {
                using (var xmlReader = XmlReader.Create(stringReader, settings))
                {
                    doc.Load(xmlReader);
                    var nav = doc.CreateNavigator();

                    XPathNavigator directoryNameNode = null;
                    try
                    {
                        var nsm = new XmlNamespaceManager(doc.NameTable);
                        nsm.AddNamespace("ns", doc.DocumentElement.NamespaceURI);
                        directoryNameNode = nav.SelectSingleNode(
                            "/ns:ClusterManifest/ns:FabricSettings/ns:Section[ns:Parameter[@Name='ConsumerType' and @Value='MdsEtwEventUploader']]/ns:Parameter[@Name='DirectoryName']",
                            nsm);
                    }
                    catch (XPathException)
                    {
                    }

                    if (directoryNameNode == null)
                    {
                        return null;
                    }

                    return directoryNameNode.GetAttribute("Value", string.Empty);
                }
            }
        }
    }
}