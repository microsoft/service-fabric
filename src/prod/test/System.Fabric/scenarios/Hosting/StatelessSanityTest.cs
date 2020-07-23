// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Scenarios.Test.Hosting
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Test.Common.FabricUtility;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    public class StatelessSanityTest
    {
        #region Service Code
        [Serializable]
        public class MyApplication : IStatelessServiceInstance
        {
            private ServiceInitializationParameters initParams;
            private Guid partitionId;
            private long instanceId;
            
            private void WriteEventToGlobalFile(string message)
            {
                Debug.WriteLine("Writing event: " + message);

                string folderPath = this.initParams.CodePackageActivationContext.GetConfigurationPackageObject(SanityTest.ConfigurationPackageName).Settings.Sections[SanityTest.ConfigurationSectionName].Parameters[SanityTest.ConfigurationParameterName].Value;
                // Only using first 4 characters from partition ID to avoid exceeding large file path
                string partitionIdSuffix = this.partitionId.ToString().Substring(0, 4);
                string filePath = Path.Combine(folderPath, string.Format("{0}_{1}.txt", partitionIdSuffix, this.instanceId));                
                File.AppendAllLines(filePath, new string[] { message });
            }

            public Threading.Tasks.Task CloseAsync(CancellationToken cancellationToken)
            {
                Debug.WriteLine("OnBeginClose");
                this.WriteEventToGlobalFile(SanityTest.CloseAsyncMarker);

                return Task.Factory.StartNew(() => { ;});
            }

            public void Abort()
            {
                Debug.WriteLine("Abort");
                this.WriteEventToGlobalFile(SanityTest.AbortMarker);
            }

            public void Initialize(StatelessServiceInitializationParameters initializationParameters)
            {
                this.initParams = initializationParameters;
                this.partitionId = initializationParameters.PartitionId;
                this.instanceId = initializationParameters.InstanceId;

                // write the information from the code package activation context
                var context = initializationParameters.CodePackageActivationContext;

                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ApplicationName = {0}", context.ApplicationName));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ApplicationTypeName = {0}", context.ApplicationTypeName));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ServiceManifestName = {0}", context.GetServiceManifestName()));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ServiceManifestVersion = {0}", context.GetServiceManifestVersion()));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "CodePackageName = {0}", context.CodePackageName));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "CodePackageVersion = {0}", context.CodePackageVersion));
            }

            public Threading.Tasks.Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken)
            {
                Debug.WriteLine("OnOpenAsync");
                Trace.WriteLine("HI");
                this.WriteEventToGlobalFile(SanityTest.OpenAsyncMarker);

                // return address as null or a string based upon the config entry.
                // null needs to be tested
                string address = null;
                if (this.initParams.CodePackageActivationContext.GetConfigurationPackageObject(SanityTest.ConfigurationPackageName).Settings.Sections[SanityTest.ConfigurationSectionName].Parameters.Contains(SanityTest.ConfigurationAddressParameterName))
                {
                    address = this.initParams.CodePackageActivationContext.GetConfigurationPackageObject(SanityTest.ConfigurationPackageName).Settings.Sections[SanityTest.ConfigurationSectionName].Parameters[SanityTest.ConfigurationAddressParameterName].Value; 
                }

                return Task.Factory.StartNew<string>(() => address);
            }
        };
        #endregion

        public static ServiceDeployer GetService(int instanceCount)
        {
            return new ServiceDeployer()
            {
                CodePackageFiles = new List<string>(SanityTest.GetAllCodePackageBinaries()),
                InstanceCount = instanceCount,
                ServiceTypeImplementation = typeof(StatelessSanityTest.MyApplication),
                ServiceTypeName = "SanityTestStatelessService",
                MinReplicaSetSize = 2, // these need to be 2 for manifest etc to generate correctly (TODO investigate)
                TargetReplicaSetSize = 2
            };
        }
    }
}