// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.IO;
    using System.Reflection;

    internal class Constant
    {
        internal static string TestDirectory = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;
        internal const string WrpSettingsFilename = "WRPSettings.xml";
        internal const string ClusterSettingsFileName = "ClusterSettings.json";
        public const string ConfigurationsFileName = "Configurations.csv";
    }
}