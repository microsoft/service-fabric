// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Threading;
    using System.Fabric.Strings;
    using System.IO;
#if DotNetCoreClrLinux
    using Common.Tracing;
#endif

    public static class Program
    {
        public static int Main(string[] args)
        {
            string argsString = string.Join(" ", args);
            try
            {
                DeployerTrace.WriteInfo("Deployer called with {0}", argsString);
                DeploymentParameters parameters = CommandLineInfo.Parse(args);
                DeploymentOperation.ExecuteOperation(parameters, false);
                return Constants.ErrorCode_Success;
            }
            catch (FabricHostRestartRequiredException e)
            {
                DeployerTrace.WriteInfo(e.ToString());
                return Constants.ErrorCode_RestartRequired;                
            }
            catch (FabricHostRestartNotRequiredException e)
            {
                DeployerTrace.WriteInfo(e.ToString());
                return Constants.ErrorCode_RestartNotRequired;
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("Deployer failed with args: {0}", argsString);
                DeployerTrace.WriteError(e.ToString());
                return Constants.ErrorCode_Failure;
            }
        }
    }
}