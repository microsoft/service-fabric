// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CommandLineParser;

namespace AppManifestCleanupUtil
{
    class CommandArgs
    {
        [CommandLineArgument(
           CommandLineArgumentType.AtMostOnce,
           Description = "Application Manifest Path to remove unnecessary Service Manifest references from.",
           LongName = "ApplicationManifestPath",
           ShortName = "amp")]
        public string ApplicationManifestPath;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description = "A semicolon separated list of valid service manifest.",
            LongName = "ServiceManifestPathList",
            ShortName = "smpl")]
        public string ServiceManifestPathList;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description = "A semicolon separated list of application parameters files used for deployment by Visual Studio. The file contains contains app name and parameter values.",
            LongName = "ApplicationParametersFiles",
            ShortName = "appparamfiles")]
        public string ApplicationParametersFilePathList;

        public CommandArgs()
        {
            this.ApplicationManifestPath = null;
            this.ServiceManifestPathList = null;
            this.ApplicationParametersFilePathList = null;
        }
    }
}