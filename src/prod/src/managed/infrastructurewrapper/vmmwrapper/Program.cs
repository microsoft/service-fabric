// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.VMMWrapper
{
    using System;
    using System.IO;
    using InfrastructureWrapper;
    using System.Text;

    public class Program
    {
        public static int Main(string[] args)
        {
            Logger.LogInfo("Main", "VMMWrapper being called");

            if (args.Length < 1)
            {
                Logger.LogError("Main", "Cluster manifest location is a required parameter.");
                return (int)ErrorCode.Fail;
            }

            if (args[0].Equals("Remove", StringComparison.CurrentCultureIgnoreCase) )
            {
                Logger.LogInfo("Main", "VMMWrapper Remove called");
                return VMMWrapper.OnVMRemove();
            }            

            string inputCMLocation = args[0];
            if (!File.Exists(inputCMLocation))
            {
                Logger.LogError("Main", "Input Cluster manifest {0} not found", inputCMLocation);
                return (int)ErrorCode.ClusterManifestInvalidLocation;
            }
        
            string inputVMUD = Console.ReadLine();
            //string inputVMUD = @"SingleTier1[mixuVMM003.redmond.corp.microsoft.com:1,mixuVMM002.redmond.corp.microsoft.com:0,mixuVMM001.redmond.corp.microsoft.com:2]";                    
            Logger.LogInfo("Main", "Input ServiceVMComputerName is {0}", inputVMUD);

            if (inputVMUD.Trim().Length == 0)
            {
                Logger.LogError("Main", "Input stream [{0}] did not provide the node list", inputVMUD);
                return (int)ErrorCode.InvalidNodeList;
            }

            VMMWrapper installer = new VMMWrapper(inputVMUD, inputCMLocation);
            return installer.OnStart();
        }       
    }
}