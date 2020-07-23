// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.IO;
    using System.Text;
    using System.Xml;
    using System.Xml.Serialization;

    internal static class XmlUtility
    {
        public static string Serialize<T>(T value) where T : class
        {
            if (value == default(T))
            {
                return null;
            }

            var serializer = new XmlSerializer(typeof(T));

            using (var memoryStream = new MemoryStream())
            {
                using (var xmlWriter = XmlWriter.Create(memoryStream, GetXmlWriterSettings()))
                {
                    serializer.Serialize(xmlWriter, value);
                }

                memoryStream.Flush();
                return Encoding.UTF8.GetString(memoryStream.ToArray());
            }
        }

        public static T Deserialize<T>(string contents) where T : class
        {
            if (string.IsNullOrEmpty(contents))
            {
                return null;
            }

            var serializer = new XmlSerializer(typeof(T));
            using (var stringReader = new StringReader(contents))
            {
                var settings = new XmlReaderSettings();
                settings.XmlResolver = null;

                using (var xmlReader = XmlReader.Create(stringReader, settings))
                {
                    return (T) serializer.Deserialize(xmlReader);
                }
            }
        }

        public static XmlWriterSettings GetXmlWriterSettings()
        {
            return new XmlWriterSettings
            {
                Encoding = new UTF8Encoding(false),
                Indent = true,
                OmitXmlDeclaration = false
            };
        }
    }
}