// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System.Threading;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.FabricDeployer;
    using System.Fabric.ImageStore;
    using System.Fabric.Management.ImageBuilder;
    using System.Fabric.Management.ImageBuilder.DockerCompose;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Management.Automation.Runspaces;
    using WEX.TestExecution;
    using YamlDotNet.RepresentationModel;

    [TestClass]
    public class DockerComposeParserTest
    {
        //
        // The result from parsing the volumes here is compared against the tuple array below.
        //
        private const string validServiceVolumes = @"
  volumes:
  - c:\foo:d:\bar
  - c:\foo bar:d:\bar
  - c:\foo bar:d:rw
  - c:\foo bar:d:\:ro
  - c:\foo:d::ro
  - mydriver:d:\
  - mydriver:d::ro
  - mydriver:d:\foo";

        // Linux style path specification to be tested later.
        //
        //   - logvolume01:/var/log
        //   - /var/log:/var/log1:ro
        //

        // Host, container, ro
        private Tuple<string, string, bool>[] parsedValidVolumes =
        {
            Tuple.Create(@"c:\foo", @"d:\bar", false),
            Tuple.Create(@"c:\foo bar", @"d:\bar", false),
            Tuple.Create(@"c:\foo bar", @"d:", false),
            Tuple.Create(@"c:\foo bar", @"d:\", true),
            Tuple.Create(@"c:\foo", @"d:", true),
            Tuple.Create(@"mydriver", @"d:\", false),
            Tuple.Create(@"mydriver", @"d:", true),
            Tuple.Create(@"mydriver", @"d:\foo", false),
            Tuple.Create(@"logvolume01", @"/var/log", false),
            Tuple.Create(@"/var/log", @"/var//var/log1", true),
        };

        private TestContext testContextInstance;
        private ImageBuilderTestContext imageBuilderTestContext;
        private ImageBuilder imageBuilder;
        private IImageStore imageStore;

        public TestContext TestContext
        {
            get { return testContextInstance; }
            set { testContextInstance = value; }
        }

        [TestInitialize]
        public void TestSetup()
        {
            this.imageBuilderTestContext = ImageBuilderTestContext.Create(this.TestContext.TestName);
            this.imageBuilder = imageBuilderTestContext.ImageBuilder;
            this.imageStore = imageBuilderTestContext.ImageStore;
            //CreateDropAndSetFabricCodePath();
        }

        [TestCleanup]
        public virtual void TestCleanup()
        {
            this.imageBuilderTestContext.TestCleanup();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void Services_test()
        {
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string yamlFilePath = Path.Combine(currentExecutingDirectory, "services_test.yml");

            var rootNode = this.GetRootNodeFromFile(yamlFilePath);
            foreach (var item in rootNode.Children)
            {
                switch (item.Key.ToString())
                {
                    case DockerComposeConstants.ServicesKey:
                    {
                        this.TestServicesKey(item.Value);
                        break;
                    }
                    case DockerComposeConstants.PortsKey:
                    {
                        this.TestPorts(item.Value);
                        break;
                    }
                    case DockerComposeConstants.DeployKey:
                    {
                        this.TestDeploy(item.Value);
                        break;
                    }
                    case DockerComposeConstants.LoggingKey:
                    {
                        this.TestLogging(item.Value);
                        break;
                    }
                    default:
                    {
                        throw new ArgumentException(string.Format("Test for key {0} not present - file: {1}", item.Key.ToString(), yamlFilePath));
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceVolume_test()
        {
            var volumesNode = (YamlSequenceNode)this.GetRootNode(validServiceVolumes).Children["volumes"];
            int i = 0;
            HashSet<string> ignoredKeys = new HashSet<string>();
            foreach (var item in volumesNode)
            {
                var volumeMapping = new ComposeServiceVolume();
                volumeMapping.Parse(item.ToString(), item, ignoredKeys);
                Verify.IsTrue(
                    string.Equals(volumeMapping.HostLocation, this.parsedValidVolumes[i].Item1),
                    volumeMapping.HostLocation);
                Verify.IsTrue(
                    string.Equals(volumeMapping.ContainerLocation, this.parsedValidVolumes[i].Item2),
                    volumeMapping.ContainerLocation);
                Verify.IsTrue(volumeMapping.ReadOnly == this.parsedValidVolumes[i].Item3, item.ToString());

                ++i;
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AllOptions_test()
        {
            string currentExecutingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string yamlFilePath = Path.Combine(currentExecutingDirectory, "all_supported_options.yml");
            string tempOutputFile = Path.GetTempFileName();

            string tempPath = Path.Combine(TestUtility.TestDirectory, Guid.NewGuid().ToString());

            this.imageBuilder.BuildComposeDeploymentPackage(
                yamlFilePath, 
                null, 
                TimeSpan.MaxValue,
                "fabric:/TestApps/TestApp1",
                "TestApplicationType",
                "v1",
                "TestUser",
                "TestPassword",
                false,
                true,
                tempOutputFile,
                tempPath, // output pkg build path
                false);

            BuildLayoutInfo buildLayoutInfo = new BuildLayoutInfo(this.imageStore, tempPath);
            TestUtility.VerifyStoreLayout(buildLayoutInfo);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceDnsName_test()
        {
            string randomLargeStringWithDot =
                "asdfasdvasdvan.vdpavpansvdasndvaindvoiasdoaihfeaodfasdfaraesfadvasdfasawtarasrasaifjapsjdfapdfpaisjdfpaisjfdpaisjdfapijdpagakldnaspojsgpaojsgpiawasdfasdvasdvanvdpavpansvdasndvaindvoiasdoaihfeaodfasdfaraesfadvasdfasawtarasrasaifjapsjdfapdfpaisjdfpaisjfdpaisjdfapijdpagakldnaspojsgpaojsgpiaw";
            string charWith62Chars = "asdfasdvasdvanvdpavpansvdasndvaindvoiasdoaihfeaodfasdfaraesfa";

            // Large service FQDN
            Verify.Throws<FabricComposeException>(
                () =>
                {
                    DockerComposeUtils.GetServiceDnsName("", randomLargeStringWithDot);
                });

            // Small serviceSuffix with >63 char appname segment
            Uri fabricName = new Uri("fabric:/MyApp/" + charWith62Chars + "foobar");
            Verify.Throws<FabricComposeException>(
                () =>
                {
                    DockerComposeUtils.GetServiceDnsName(fabricName.ToString(), "myservice");
                });

            // total length exceeding 255
            fabricName = new Uri("fabric:/MyApp/" + charWith62Chars + "/" + charWith62Chars + "/" + charWith62Chars + "/" + charWith62Chars);
            Verify.Throws<FabricComposeException>(
                () =>
                {
                    DockerComposeUtils.GetServiceDnsName(fabricName.ToString(), charWith62Chars);
                });

            string expectedDnsNameResult = "myservice." + charWith62Chars + ".myapp";
            fabricName = new Uri("fabric:/myapp/" + charWith62Chars);
            var result = DockerComposeUtils.GetServiceDnsName(fabricName.ToString(), "myservice");

            Verify.IsTrue(result == expectedDnsNameResult, string.Format("{0} == {1}", result, expectedDnsNameResult));

            expectedDnsNameResult = "myservice.";
            result = DockerComposeUtils.GetServiceDnsName(fabricName.ToString(), expectedDnsNameResult);
            Verify.IsTrue(result == expectedDnsNameResult, string.Format("{0} == {1}", result, expectedDnsNameResult));
        }

        private void TestServicesKey(YamlNode servicesNode)
        {
            var services = (YamlMappingNode) servicesNode;
            HashSet<string> ignoredKeys = new HashSet<string>();
            foreach (var item in services.Children)
            {
                Console.WriteLine(item.Key.ToString());
                if (item.Key.ToString().StartsWith("valid"))
                {
                    var serviceTypeDecsription = new ComposeServiceTypeDescription(item.Key.ToString(), "latest");
                    serviceTypeDecsription.Parse(DockerComposeUtils.GenerateTraceContext("services", item.Key.ToString()), item.Value, ignoredKeys);
                    serviceTypeDecsription.Validate();
                }
                else
                {
                    var serviceTypeDecsription = new ComposeServiceTypeDescription(item.Key.ToString(), "latest");
                    Verify.Throws<FabricComposeException>(
                        () =>
                        {
                            serviceTypeDecsription.Parse(
                                DockerComposeUtils.GenerateTraceContext("services", item.Key.ToString()), item.Value, ignoredKeys);
                            serviceTypeDecsription.Validate();
                        });
                }
            }            
        }

        private void TestPorts(YamlNode portsNode)
        {
            var portMappings = (YamlSequenceNode) portsNode;
            HashSet<string> ignoredKeys = new HashSet<string>();
            foreach (var item in portMappings)
            {
                Console.WriteLine(item);
                var portMapping = new ComposeServicePorts();
                Verify.Throws<FabricComposeException>(
                    () =>
                    {
                        portMapping.Parse(DockerComposeUtils.GenerateTraceContext("ports", item.ToString()), item, ignoredKeys);
                        portMapping.Validate();
                    });
            }
        }

        private void TestDeploy(YamlNode node)
        {
            var deployNodes = (YamlMappingNode)node;
            HashSet<string> ignoredKeys = new HashSet<string>();
            foreach (var item in deployNodes)
            {
                Console.WriteLine(item.Key.ToString());
                if (item.Key.ToString().StartsWith("valid"))
                {
                    var serviceTypeDecsription = new ComposeServiceTypeDescription(item.Key.ToString(), "latest");
                    serviceTypeDecsription.ParseDeploymentParameters(
                        DockerComposeUtils.GenerateTraceContext("services.Deploy", item.Key.ToString()), 
                        (YamlMappingNode)item.Value,
                        ignoredKeys);

                    serviceTypeDecsription.ImageName = "test";
                    serviceTypeDecsription.Validate();
                }
                else
                {
                    var serviceTypeDecsription = new ComposeServiceTypeDescription(item.Key.ToString(), "latest");
                    Verify.Throws<FabricComposeException>(
                        () =>
                        {
                            serviceTypeDecsription.ParseDeploymentParameters(
                                DockerComposeUtils.GenerateTraceContext("services", item.Key.ToString()),
                                (YamlMappingNode)item.Value,
                                ignoredKeys);
                            serviceTypeDecsription.Validate();
                        });

                }
            }
        }

        private void TestLogging(YamlNode node)
        {
            var loggingNodes = (YamlMappingNode) node;
            HashSet<string> ignoredKeys = new HashSet<string>();
            foreach (var item in loggingNodes)
            {
                Console.WriteLine(item.Key.ToString());
                if (item.Key.ToString().StartsWith("valid"))
                {
                    var loggingOptions = new ComposeServiceLoggingOptions();
                    loggingOptions.Parse(item.Key.ToString(), item.Value, ignoredKeys);
                    loggingOptions.Validate();
                }
                else
                {
                    var loggingOptions = new ComposeServiceLoggingOptions();
                    Verify.Throws<FabricComposeException>(
                        () =>
                        {
                            loggingOptions.Parse(
                                DockerComposeUtils.GenerateTraceContext("logging", item.Key.ToString()), item.Value, ignoredKeys);
                            loggingOptions.Validate();
                        });

                }
            }
        }

        private YamlMappingNode GetRootNodeFromFile(string yamlFilepath)
        {
            var yamlStream = DockerComposeUtils.ReadYamlFile(yamlFilepath);
            return (YamlMappingNode)yamlStream.Documents[0].RootNode;
        }

        private YamlMappingNode GetRootNode(string input)
        {
            var yamlStream = new YamlStream();
            yamlStream.Load(new StringReader(input));
            return (YamlMappingNode)yamlStream.Documents[0].RootNode;
        }

        internal static void CreateDropAndSetFabricCodePath()
        {
            var RootLocation = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "log", "FD.Tests");
            var DropLocation = "Drop";
            if (!Directory.Exists(RootLocation))
            {
                Directory.CreateDirectory(RootLocation);
            }

            if (!Directory.Exists(DropLocation))
            {
                string makeDropScriptPath = Environment.ExpandEnvironmentVariables(@"%SRCROOT%\prod\Setup\scripts\MakeDrop.cmd");
                if (File.Exists(makeDropScriptPath))
                {
                    int exitCode = FabricDeployerDeploymentTests.ExecuteProcess(
                        makeDropScriptPath,
                        string.Format(CultureInfo.InvariantCulture, @"/dest:{0} /symbolsDestination:nul", DropLocation));
                    Verify.AreEqual<int>(0, exitCode, "Unable to create Drop");
                }
                else
                {
                    string fabricDropPath = Environment.ExpandEnvironmentVariables(@"%_NTTREE%\FabricDrop");
                    FabricDeployerDeploymentTests.DirectoryCopy(fabricDropPath, DropLocation, true);
                }
            }

            string fabricCodePackage = null;
            string fabricAppDirectory = Path.Combine(Directory.GetCurrentDirectory(), DropLocation, @"bin\Fabric");
            foreach (string dir in Directory.EnumerateDirectories(fabricAppDirectory, "Fabric.Code"))
            {
                fabricCodePackage = dir;
            }

            string path = Environment.GetEnvironmentVariable("path");
            path = string.Format(CultureInfo.InvariantCulture, "{0};{1}", path, fabricCodePackage);
            Environment.SetEnvironmentVariable("path", path);
            Environment.SetEnvironmentVariable("FabricCodePath", fabricCodePackage);
        }
    }
}