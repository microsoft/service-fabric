// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Utils
{
    using System;
    using System.Fabric;
    using Microsoft.ServiceFabric.Data;

    internal class ServiceReplicaUtils
    {
        internal static string GetDedicatedLogPath(StatefulServiceContext initializationParameters)
        {
            return initializationParameters.CodePackageActivationContext.WorkDirectory;
        }

        internal static void SetupLoggerPath(
            ref ReliableStateManagerReplicatorSettings replicatorSettings,
            ref StatefulServiceInitializationParameters initializationParameters)
        {

            string pathToSharedContainer;

            // Use this when running with the kernel mode logger as all replicas for all nodes should use the same log
            if (string.IsNullOrEmpty(replicatorSettings.SharedLogId)
                || string.IsNullOrEmpty(replicatorSettings.SharedLogPath))
            {

                string fabricDataRoot;

                try
                {
                    fabricDataRoot = System.Fabric.Common.FabricEnvironment.GetDataRoot();
                }
                catch (Exception)
                {
                    //
                    // If the Fabric Data Root does not exist then we may be running in a container. And in
                    // this case we count on the driver to do the mapping into the host path. So pick a random
                    // path here knowing it will be mapped.
                    //
                    fabricDataRoot = "q:\\";
                }

                // Use default log id and location {3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}
                var staticLogContainerGuid = Constants.DefaultSharedLogContainerGuid;

                pathToSharedContainer = System.IO.Path.Combine(
                    fabricDataRoot,
                    Constants.DefaultSharedLogSubdirectory);
                pathToSharedContainer = System.IO.Path.Combine(pathToSharedContainer, Constants.DefaultSharedLogName);

                replicatorSettings.SharedLogId = staticLogContainerGuid;
                replicatorSettings.SharedLogPath = pathToSharedContainer;
            }
        }
    }
}