// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System;
    using System.Fabric.Common;
    using System.Threading;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Net;

    public class Program
    {
        private const string TraceType = "BackupRestoreService";
        private const string BackupRestoreServiceType = "BackupRestoreServiceType";

        internal static FabricBackupRestoreServiceAgent ServiceAgent
        {
            get;
            private set;
        }

        public static void Main(string[] args)
        {

#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif
            try
            {
                ServiceRuntime.RegisterServiceAsync(
                    BackupRestoreServiceType,
                    ReadConfiguration).GetAwaiter().GetResult();

                Program.ServiceAgent = FabricBackupRestoreServiceAgent.Instance;
                Thread.Sleep(Timeout.Infinite);
            }
            catch (Exception e)
            {
                BackupRestoreTrace.TraceSource.WriteError(TraceType, "Host failed: " + e);
                Console.WriteLine("Exception encountered {0}", e);
                throw;
            }
        }

        private static StatefulServiceBase ReadConfiguration(StatefulServiceContext context)
        {
            var configStore = NativeConfigStore.FabricGetConfigStore();
            var backupRestoreServiceReplicatorAddress = configStore.ReadUnencryptedString("FabricNode", "BackupRestoreServiceReplicatorAddress");
            
            return new BackupRestoreService(context, backupRestoreServiceReplicatorAddress);
        }
    }
}