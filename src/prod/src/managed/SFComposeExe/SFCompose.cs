// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace SFCompose
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ImageBuilder.DockerCompose;
    using System.Fabric.Management.ImageBuilder.SingleInstance;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.ImageBuilderExe;
    using System.IO;
    using System.Linq;
    using CommandLineParser;
    using YamlDotNet.RepresentationModel;
    using Newtonsoft.Json;

    internal class SFCompose
    {
        public static int Main(string[] args)
        {
            var parsedArguments = new Arguments();
            if (!CommandLineUtility.ParseCommandLineArguments(args, parsedArguments) || !parsedArguments.IsValid())
            {
                Console.Write(CommandLineUtility.CommandLineArgumentsUsage(typeof(Arguments)));
                return -1;
            }

            if (!parsedArguments.VolumeDescriptionRequired)
            {
                VolumeMount.TestSettingVolumeDescriptionRequired = false;
            }

            if (parsedArguments.Operation == Arguments.Type.SingleInstance)
            {
                ApplicationOperation(parsedArguments);
            }
            else
            {
                ComposeOperation(parsedArguments);
            }
            return 0;
        }

        private static void ComposeOperation(Arguments parsedArguments)
        {
            try
            {
                var stream = ReadInputFile(parsedArguments.DockerComposeFilePath);
                var result = new ComposeApplicationTypeDescription();
                HashSet<string> ignoredKeys = new HashSet<string>();
                result.Parse("", stream.Documents[0].RootNode, ignoredKeys);
                result.Validate();

                ApplicationManifestType appManifest;
                IList<ServiceManifestType> serviceManifest;

                var generator = new ContainerPackageGenerator(
                    result,
                    new ContainerRepositoryCredentials(),
                    "",
                    true);
                generator.Generate(out appManifest, out serviceManifest);

                if (parsedArguments.Operation == Arguments.Type.Generate)
                {
                    DockerComposeUtils.GenerateApplicationPackage(appManifest, serviceManifest, parsedArguments.GeneratedApplicationPackagePath);
                }

            }
            catch (Exception e)
            {
                Console.WriteLine(e);
            }
        }

        private static void ApplicationOperation(Arguments parsedArguments)
        {
            if (String.IsNullOrEmpty(parsedArguments.ApplicationDescriptionPath) ||
                String.IsNullOrEmpty(parsedArguments.GeneratedApplicationPackagePath))
            {
                Console.WriteLine("ApplicationDescriptionPath and GeneratedApplicationPackagePath should be specified for SingleInstance");
                return;
            }
            
            try
            {
                Application application = null;
                using (StreamReader file = File.OpenText(parsedArguments.ApplicationDescriptionPath))
                {
                    JsonSerializer serializer = new JsonSerializer();
                    serializer.Converters.Add(new DiagnosticsSinkJsonConverter());
                    serializer.Converters.Add(new VolumeMountJsonConverter());

                    application =
                        (Application)serializer.Deserialize(file, typeof(Application));
                }

                ApplicationManifestType appManifest;
                IList<ServiceManifestType> serviceManifest;
                
                GenerationConfig config = new GenerationConfig() { RemoveServiceFabricRuntimeAccess = true, IsolationLevel = "hyperV" };
                var generator = new ApplicationPackageGenerator("TestType", "1.0", application, true, false, "C:\\settings", true, config);
                Dictionary<string, Dictionary<string, SettingsType>> settingsType;
                generator.Generate(out appManifest, out serviceManifest, out settingsType);
                DockerComposeUtils.GenerateApplicationPackage(appManifest, serviceManifest, settingsType, parsedArguments.GeneratedApplicationPackagePath);
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
            }
        }

        private static YamlStream ReadInputFile(string filename)
        {
            var yamlStream = new YamlStream();
            try
            {
                using (StreamReader reader = new StreamReader(new FileStream(filename, FileMode.Open)))
                {
                    yamlStream.Load(reader);
                }
            }
            catch (Exception)
            {
                // TODO: Log
                throw;
            }

            return yamlStream;
        }
    }
}