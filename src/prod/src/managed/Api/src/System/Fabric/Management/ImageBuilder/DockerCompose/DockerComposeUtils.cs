// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;
    using YamlDotNet.RepresentationModel;

#if DotNetCoreClrLinux
#if !StandaloneTool
using System.Fabric.Common;
#endif
#endif

    /// <summary>
    /// A simple utility class for generating the type and package names. The methods that get the application manifest 
    /// and service manifest paths should be removed once we have a common standard way for runtime and standalone client 
    /// components to access the buildlayoutspecification.
    /// </summary>
    internal class DockerComposeUtils
    {
        private const string ApplicationManifestFileName = "ApplicationManifest.xml";
        private const string ServiceManifestFileName = "ServiceManifest.xml";
        private const string SettingXmlFileName = "Settings.xml";
        private const int MaxAttempts = 60;
        private const int MaxRetryIntervalMillis = 1000;
        private const int MinRetryIntervalMillis = 100;
        private static readonly Random Rand = new Random();

        public static string GetServiceTypeName(string name)
        {
            return String.Format(CultureInfo.InvariantCulture, "{0}Type", name);
        }

        public static string GetServicePackageName(string name)
        {
            return String.Format(CultureInfo.InvariantCulture, "{0}Pkg", name);
        }

        public static string GetEndpointName(string name, int index)
        {
            if (index != 0)
            {
                return String.Format(CultureInfo.InvariantCulture, "{0}Endpoint{1}", name, index);
            }
            else
            {
                return String.Format(CultureInfo.InvariantCulture, "{0}Endpoint", name);
            }
        }

        public static string GetSettingXmlFilePath(string appPackagePath, string servicePackageName, string configPackageName)
        {
            return Path.Combine(appPackagePath, servicePackageName, configPackageName, SettingXmlFileName);
        }

        public static string GetCodePackageName(string name)
        {
            return String.Format(CultureInfo.InvariantCulture, "{0}.Code", name);
        }

        public static string GetServiceManifestFilePath(string appPackagePath, string servicePackageName)
        {
            return Path.Combine(appPackagePath, servicePackageName, ServiceManifestFileName);
        }

        public static string GetApplicationManifestFilePath(string appPackagePath)
        {
            return Path.Combine(appPackagePath, ApplicationManifestFileName);
        }

        public static void GenerateApplicationPackage(
            ApplicationManifestType applicationManifest,
            IList<ServiceManifestType> serviceManifests,
            Dictionary<string, Dictionary<string, SettingsType>> settingsType,
            string packageOutputLocation)
        {
            GenerateApplicationPackage(applicationManifest, serviceManifests, packageOutputLocation);

            foreach (var settingsByService in settingsType)
            {
                string serviceManifestName = settingsByService.Key;
                foreach (var settingsTypeByConfig in settingsByService.Value)
                {
                    string configPackageName = settingsTypeByConfig.Key;
                    WriteFile(GetSettingXmlFilePath(packageOutputLocation, serviceManifestName, configPackageName), SerializeXml(settingsTypeByConfig.Value));
                }
            }
        }

        public static void GenerateApplicationPackage(
            ApplicationManifestType applicationManifest,
            IList<ServiceManifestType> serviceManifests,
            string packageOutputLocation)
        {
            WriteFile(GetApplicationManifestFilePath(packageOutputLocation), SerializeXml(applicationManifest));

            foreach (var serviceManifest in serviceManifests)
            {
                WriteFile(GetServiceManifestFilePath(packageOutputLocation, serviceManifest.Name), SerializeXml(serviceManifest));
            }
        }

        public static string GetServiceDnsName(string applicationName, string serviceSuffix)
        {
            var serviceDnsName = serviceSuffix;

            //
            // If the dns service suffix is not already a fully qualified domain name, create the name under application's domain to avoid 
            // dns name conflicts with services in other applications.
            //
            if (!serviceSuffix.Contains(".") && !String.IsNullOrEmpty(applicationName))
            {
                //
                // Each of the path segment in the application name becomes a domain label in the service Dns Name, With the first path segment
                // becoming the top level domain label.
                //
                var appNameUri = new Uri(applicationName);
                var appNameSegments = appNameUri.AbsolutePath.Split('/');
                for (int i = appNameSegments.Length - 1; i >= 0; --i)
                {
                    // Each segment max length is 63
                    if (appNameSegments[i].Length > 63)
                    {
                        throw new FabricComposeException(
                            String.Format(
                                "Service DNS name cannot be generated. Segment '{0}' of application name '{1}' exceeds max domain label size.",
                                appNameSegments[i],
                                appNameUri));
                    }

                    if (!String.IsNullOrEmpty(appNameSegments[i]))
                    {
                        serviceDnsName += "." + appNameSegments[i];
                    }
                }
            }

            //
            // Total length of the domain name cannot be > 255 characters - RFC 1035
            //
            if (serviceDnsName.Length > 255)
            {
                throw new FabricComposeException(
                    String.Format(
                        "Service DNS name cannot be generated. The dns name '{0}' exceeds max dns name length.",
                        serviceDnsName));
            }

            return serviceDnsName;
        }

        private static void WriteFile(string filePath, string contents)
        {
            EnsureFolder(Path.GetDirectoryName(filePath));
            using (var fileStream = GetExclusiveFilestream(filePath, FileMode.Create))
            {
                WriteContents(fileStream, contents);
            }
        }

        public static YamlNode GetRootNode(string fileName)
        {
            var yamlStream = ReadYamlFile(fileName);
            
            // Return the root of the first document.
            return yamlStream.Documents[0].RootNode;
        }

        public static YamlStream ReadYamlFile(string fileName)
        {
            var yamlStream = new YamlStream();
            try
            {
                using (StreamReader reader = new StreamReader(new FileStream(fileName, FileMode.Open, FileAccess.Read)))
                {
                    yamlStream.Load(reader);
                }
            }
            catch (Exception e)
            {
                throw new FabricComposeException(String.Format("Exception when parsing yml file {0}", fileName), e);
            }

            return yamlStream;

        }

        public static string SerializeXml<T>(T value) where T : class
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

        public static T DeserializeXml<T>(string contents) where T : class
        {
            if (string.IsNullOrEmpty(contents))
            {
                return null;
            }

            var serializer = new XmlSerializer(typeof(T));
            using (var stringReader = new StringReader(contents))
            {
                var settings = new XmlReaderSettings();
#if !DotNetCoreClr
                settings.XmlResolver = null;
#endif

                using (var xmlReader = XmlReader.Create(stringReader, settings))
                {
                    return (T) serializer.Deserialize(xmlReader);
                }
            }
        }

        private static XmlWriterSettings GetXmlWriterSettings()
        {
            return new XmlWriterSettings
            {
                Encoding = new UTF8Encoding(false),
                Indent = true,
                OmitXmlDeclaration = false
            };

        }

        private static FileStream GetExclusiveFilestream(string fileName, FileMode fileMode = FileMode.Create)
        {
            FileStream fileStream = null;
            for (var i = 0; ; i += 1)
            {
                try
                {
                    fileStream = new FileStream(fileName, fileMode, FileAccess.ReadWrite, FileShare.Read);
                    break;
                }
                catch (IOException e)
                {
                    if (i == MaxAttempts)
                        throw new FabricComposeException(string.Format("Unable to create {0}", fileName), e);
                }

                Thread.Sleep(TimeSpan.FromMilliseconds(Rand.Next(MinRetryIntervalMillis, MaxRetryIntervalMillis)));
            }
#if DotNetCoreClrLinux
#if !StandaloneTool
            Helpers.UpdateFilePermission(fileName);
#endif
#endif
            return fileStream;
        }

        private static void WriteContents(FileStream stream, string contents)
        {
            var sb = new StringBuilder(contents);
            sb.AppendLine();
            sb.Append(' ');
            contents = sb.ToString();

            stream.Seek(0, SeekOrigin.Begin);
            using (var writer = new StreamWriter(stream, Encoding.UTF8, 4096, true))
            {
                writer.Write(contents);
                writer.Flush();

                var contentLength = writer.Encoding.GetByteCount(contents) + writer.Encoding.GetPreamble().Length;

                stream.SetLength(contentLength);
                stream.Flush();
            }
        }

        private static void EnsureFolder(string folderPath)
        {
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
        }

        public static string GenerateTraceContext(string parentContext, string currentContext)
        {
            if (string.IsNullOrEmpty(parentContext))
            {
                return currentContext;
            }
            return $"{parentContext}.{currentContext}";
        }

        public static YamlScalarNode ValidateAndGetScalar(string traceContext, string name, YamlNode inputNode)
        {
            if (inputNode == null || inputNode.NodeType != YamlNodeType.Scalar)
            {
                throw new FabricComposeException($"{traceContext} - {name} expects a scalar value");
            }

            return (YamlScalarNode) inputNode;
        }

        public static YamlMappingNode ValidateAndGetMapping(string traceContext, string name, YamlNode inputNode)
        {
            if (inputNode == null || inputNode.NodeType != YamlNodeType.Mapping)
            {
                throw new FabricComposeException($"{traceContext} - {name} expects a mapping");
            }

            return (YamlMappingNode) inputNode;
        }

        public static YamlSequenceNode ValidateAndGetSequence(string traceContext, string name, YamlNode inputNode)
        {
            if (inputNode == null || inputNode.NodeType != YamlNodeType.Sequence)
            {
                throw new FabricComposeException($"{traceContext} - {name} expects a sequence");
            }

            return (YamlSequenceNode)inputNode;
        }
    }
}