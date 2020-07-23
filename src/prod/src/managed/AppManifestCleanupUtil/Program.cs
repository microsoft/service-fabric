// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace AppManifestCleanupUtil
{
    using System;
    using System.Collections.Generic;
    using CommandLineParser;

    class Program
    {
        static int Main(string[] args)
        {
            var parser = new CommandLineArgumentParser(typeof(CommandArgs), new ErrorReporter(Console.Error.WriteLine));
            var cmdArgs = new CommandArgs();

            if (!parser.Parse(args, cmdArgs))
            {
                Console.Write(CommandLineUtility.CommandLineArgumentsUsage(typeof(CommandArgs)));
                return -1;
            }

            string appManifestPath = cmdArgs.ApplicationManifestPath;
            List<string> serviceManifestPaths = new List<string>(cmdArgs.ServiceManifestPathList.Split(';'));
            AppManifestCleanupUtil util = new AppManifestCleanupUtil();

            List<string> appParamFilePaths = null;
            if (cmdArgs.ApplicationParametersFilePathList != null)
            {
                appParamFilePaths = new List<string>(cmdArgs.ApplicationParametersFilePathList.Split(';'));
            }

            util.CleanUp(appManifestPath, serviceManifestPaths, appParamFilePaths);

            return 0;
        }
    }
}