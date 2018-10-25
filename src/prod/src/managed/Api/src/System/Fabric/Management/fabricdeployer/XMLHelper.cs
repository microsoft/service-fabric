// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if DotNetCoreClr
using System.Fabric.Common;
#endif
using System.IO;
using System.Text;
using System.Threading;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;

namespace System.Fabric.FabricDeployer
{
    class XmlHelper
    {

        internal static T ReadXml<T>(string fileName, string schemaFile)
        {
            XmlReaderSettings schemaValidatingReaderSettings = new XmlReaderSettings();
#if !DotNetCoreClr
            schemaValidatingReaderSettings.ValidationType = ValidationType.Schema;
            schemaValidatingReaderSettings.XmlResolver = null;
            schemaValidatingReaderSettings.Schemas.Add(Constants.FabricNamespace, schemaFile);
#endif

            using (FileStream stream = File.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (XmlReader validatingReader = XmlReader.Create(stream, schemaValidatingReaderSettings))
                {
                    return ReadXml<T>(validatingReader);
                }
            }
        }

        internal static T ReadXml<T>(string fileName)
        {
#if !DotNetCoreClr
            using (XmlReader reader = XmlReader.Create(fileName, new XmlReaderSettings() { XmlResolver = null }))
#else
            using (XmlReader reader = XmlReader.Create(fileName, new XmlReaderSettings()))
#endif
            {
                return ReadXml<T>(reader);
            }
        }

        internal static T ReadXml<T>(XmlReader reader)
        {
            XmlSerializer serializer = new XmlSerializer(typeof(T));
            T obj = (T)serializer.Deserialize(reader);
            return obj;
        }

        internal static void WriteXml<T>(string fileName, T value)
        {
            XmlWriterSettings settings = new XmlWriterSettings();
            settings.Indent = true;
            using (FileStream fileStream = new FileStream(fileName, FileMode.Create, FileAccess.Write, FileShare.Read))
            {
                using (XmlWriter writer = XmlWriter.Create(fileStream, settings))
                {
                    WriteXml<T>(writer, value);
                }
            }
        }

        internal static void WriteXml<T>(XmlWriter writer, T value)
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
            }
        }

        internal static FileStream GetExclusiveFilestream(string fileName, FileMode fileMode = FileMode.Create, uint retryCount = Constants.FileOpenDefaultRetryAttempts)
        {
            FileStream fileStream = null;
            for (var i = 0; ; i += 1)
            {
                try
                {
                    fileStream = new FileStream(fileName, fileMode, FileAccess.ReadWrite, FileShare.None);
                    break;
                }
                catch (IOException e)
                {
                    if (i == retryCount)
                        throw;
                    DeployerTrace.WriteWarning("Transient error - {0}", e.Message);
                }

                Thread.Sleep(TimeSpan.FromMilliseconds(i * Constants.FileOpenDefaultRetryIntervalMilliSeconds));
            }
#if DotNetCoreClrLinux
            Helpers.UpdateFilePermission(fileName);
#endif
            return fileStream;
        }
    }
}