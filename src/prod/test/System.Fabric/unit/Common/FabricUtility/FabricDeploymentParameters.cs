// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System.Collections.Generic;
    using System.IO;

    public class FabricDeploymentParameters
    {
        private FabricDeploymentParameters()
        {
            this.AdditionalFilesToDeployToNodeRoot = new List<string>();
        }

        /// <summary>
        /// The port at which deployment should start
        /// </summary>
        public int StartPort { get; set; }

        /// <summary>
        /// The root folder where this deployment is created
        /// </summary>
        public string DeploymentRootPath { get; set; }

        /// <summary>
        /// the number of nodes
        /// </summary>
        public int NodeCount { get; set; }

        /// <summary>
        /// The folder that contains the fabric binaries
        /// </summary>
        public string FabricBinariesPath { get; set; }

        /// <summary>
        /// Enabling the service state cache slows down the initial fabric deployment setup
        /// However it does provide notifications on the state of each partition for each service
        /// </summary>
        public bool EnableServiceStateCache { get; set; }

        /// <summary>
        /// Additional files to deploy to the binary folder in each node
        /// </summary>
        public IList<string> AdditionalFilesToDeployToNodeRoot { get; private set; }

        /// <summary>
        /// Create a default set of parameters
        /// The deploymentRootPath is set to the Log folder\testDllName\scenarioName
        /// </summary>
        public static FabricDeploymentParameters CreateDefault(string testDllName, string scenarioName, int nodeCount, bool enableServiceStateCache = false)
        {
            int startPort = 0;
            System.Fabric.Test.Common.FabricUtility.InProc.TestPortHelper.GetPorts(500, out startPort);

            string fabricBinPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            return new FabricDeploymentParameters
            {
                DeploymentRootPath = Path.Combine(fabricBinPath, "Log", "SFT", testDllName, scenarioName),
                FabricBinariesPath = fabricBinPath,
                NodeCount = nodeCount,
                StartPort = startPort,
                EnableServiceStateCache = enableServiceStateCache,
            };
        }
    }
}