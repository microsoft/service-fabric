// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Scenarios.Test.Hosting
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Test;
    using System.Fabric.Test.Common.FabricUtility;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class SanityTest
    {
        private const int fabricNodeCount = 1;
        private const int expectedFileCount = 1;

        public const string ConfigurationPackageName = "MyConfigPackage";

        public const string ConfigurationSectionName = "MySection";
        public const string ConfigurationParameterName = "OutputFolderPath";
        public const string ConfigurationAddressParameterName = "Address";
        public const string ConfigurationAddressParameterValue = "TestAddress";

        public const string OpenAsyncMarker = "BEGIN_OPEN";
        public const string CloseAsyncMarker = "BEGIN_CLOSE";
        public const string AbortMarker = "ABORT";

        static readonly string[] ExpectedStatefulTestOutputLines = new string[]
        {
            "ApplicationName = fabric:/Stateful",
            "ApplicationTypeName = Stateful",
            "ServiceManifestName = StatefulServicePackage",
            "ServiceManifestVersion = 1.0",
            "CodePackageName = my_code_package",
            "CodePackageVersion = 1.0",
            "BEGIN_OPEN",
            "BEGIN_CLOSE"
        };

        static readonly string[] ExpectedStatelessTestOutputLines = new string[]
        {
            "ApplicationName = fabric:/Stateless",
            "ApplicationTypeName = Stateless",
            "ServiceManifestName = StatelessServicePackage",
            "ServiceManifestVersion = 1.0",
            "CodePackageName = my_code_package",
            "CodePackageVersion = 1.0",
            "BEGIN_OPEN",
            "BEGIN_CLOSE"
        };

        public static IEnumerable<string> GetAllCodePackageBinaries()
        {
            yield return Assembly.GetExecutingAssembly().Location;
        }

        private ApplicationDeployer GetDeployer(string appName, ServiceDeployer service, string testOutputPath)
        {
            ApplicationDeployer dep = new ApplicationDeployer()
            {
                Name = appName,
            };

            service.ConfigPackageName = SanityTest.ConfigurationPackageName;
            dep.Services.Add(service);
            dep.Services[0].ConfigurationSettings.Add(Tuple.Create(SanityTest.ConfigurationSectionName, SanityTest.ConfigurationParameterName, testOutputPath));

            return dep;
        }
    }
}