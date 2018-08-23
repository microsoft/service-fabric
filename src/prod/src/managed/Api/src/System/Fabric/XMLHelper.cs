// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;

    internal class XMLHelper
    {
        /// <summary>
        /// Writes the cluster Manifest
        /// </summary>
        /// <param name="path">Path of the cluster manifest.</param>
        /// <param name="value">ClusterManifest Type.</param>
        internal static void WriteClusterManifest(string path, ClusterManifestType value)
        {
            using (FileStream fileStream = new FileStream(path, FileMode.Create))
            {
                XmlSerializer xmlSerializer = new XmlSerializer(typeof(ClusterManifestType));
                xmlSerializer.Serialize(fileStream, value);
            }
        }

        internal static ClusterManifestType ReadClusterManifest(string clusterManifestPath)
        {
            XmlReaderSettings settings = new XmlReaderSettings();
#if !DotNetCoreClr
            // For security, do not resolve external sources
            settings.XmlResolver = null;
#endif
            using (XmlReader reader = XmlReader.Create(clusterManifestPath, settings))
            {
                XmlSerializer xmlSrlizer = new XmlSerializer(typeof(ClusterManifestType));
                ClusterManifestType cm = (ClusterManifestType)xmlSrlizer.Deserialize(reader);
                return cm;
            }
        }

        internal static ClusterManifestType ReadClusterManifestFromContent(string clusterManifestContent)
        { 
            using (var reader = new StringReader(clusterManifestContent))
            {
                var xmlReaderSettings = new XmlReaderSettings();
#if !DotNetCoreClr
                xmlReaderSettings.XmlResolver = null;
#endif
                using (var xmlReader = XmlReader.Create(reader, xmlReaderSettings))
                { 
                    var serializer = new XmlSerializer(typeof(ClusterManifestType));
                    var cm = (ClusterManifestType) serializer.Deserialize(xmlReader);
                    return cm;
                }
            }
        }
        internal static InfrastructureInformationType ReadInfrastructureManifest(string infrastructureManifestPath)
        {
            XmlReaderSettings settings = new XmlReaderSettings();
#if !DotNetCoreClr
            // For security, do not resolve external sources
            settings.XmlResolver = null;
#endif
            using (XmlReader reader = XmlReader.Create(infrastructureManifestPath, settings))
            {
                XmlSerializer xmlSrlizer = new XmlSerializer(typeof(InfrastructureInformationType));
                InfrastructureInformationType im = (InfrastructureInformationType)xmlSrlizer.Deserialize(reader);
                return im;
            }
        }

        private static void WriteXml<T>(XmlWriter writer, T value)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(T));
            serializer.Serialize(writer, value);
        }

        internal static void WriteXmlExclusive<T>(string fileName, T value)
        {
            var settings = new XmlWriterSettings { Indent = true };

            using (var fs = GetExclusiveFilestream(fileName))
            {
                using (var writer = XmlWriter.Create(fs, settings))
                {
                    WriteXml<T>(writer, value);
                }
#if !DotNetCoreClr
                // Is this really required?
                fs.Close(); // Explicit close to reduce races.                
#endif
            }
        }

        private static FileStream GetExclusiveFilestream(string fileName, FileMode fileMode = FileMode.Create, uint retryCount = 100)
        {
            FileStream fileStream = null;
            for (var i = 0; ; i += 1)
            {
                try
                {
                    fileStream = new FileStream(fileName, fileMode, FileAccess.ReadWrite, FileShare.None);
                    break;
                }
                catch (IOException)
                {
                    if (i == retryCount)
                        throw;
                }

                Thread.Sleep(TimeSpan.FromMilliseconds(i * 100));
            }

            return fileStream;
        }
    }
}