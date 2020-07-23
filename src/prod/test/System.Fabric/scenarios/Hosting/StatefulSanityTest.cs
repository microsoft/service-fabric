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

    public class StatefulSanityTest
    {
        #region Service code

        [Serializable]
        public class MyApplication : IStatefulServiceReplica, IStateProvider
        {
            private ServiceInitializationParameters initParams;
            private Guid partitionId;
            private long replicaId;

            public void Initialize(StatefulServiceInitializationParameters initializationParameters)
            {
                this.initParams = initializationParameters;
                this.partitionId = initializationParameters.PartitionId;
                this.replicaId = initializationParameters.ReplicaId;

                // write the information from the code package activation context
                var context = initializationParameters.CodePackageActivationContext;

                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ApplicationName = {0}", context.ApplicationName));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ApplicationTypeName = {0}", context.ApplicationTypeName));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ServiceManifestName = {0}", context.GetServiceManifestName()));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "ServiceManifestVersion = {0}", context.GetServiceManifestVersion()));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "CodePackageName = {0}", context.CodePackageName));
                WriteEventToGlobalFile(string.Format(CultureInfo.InvariantCulture, "CodePackageVersion = {0}", context.CodePackageVersion));
            }

            private void WriteEventToGlobalFile(string message)
            {
                Debug.WriteLine("Writing event: " + message);
                string folderPath = this.initParams.CodePackageActivationContext.GetConfigurationPackageObject(SanityTest.ConfigurationPackageName).Settings.Sections[SanityTest.ConfigurationSectionName].Parameters[SanityTest.ConfigurationParameterName].Value;
                // Only using first 4 characters from partition ID to avoid exceeding large file path
                string partitionIdSuffix = this.partitionId.ToString().Substring(0, 4);
                string filePath = Path.Combine(folderPath, string.Format("{0}_{1}.txt", partitionIdSuffix, this.replicaId));
                File.AppendAllLines(filePath, new string[] { message });
            }

            public Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
            {
                // return address as null or a string based upon the config entry.
                // null needs to be tested
                string address = null;
                if (this.initParams.CodePackageActivationContext.GetConfigurationPackageObject(SanityTest.ConfigurationPackageName).Settings.Sections[SanityTest.ConfigurationSectionName].Parameters.Contains(SanityTest.ConfigurationAddressParameterName))
                {
                    address = this.initParams.CodePackageActivationContext.GetConfigurationPackageObject(SanityTest.ConfigurationPackageName).Settings.Sections[SanityTest.ConfigurationSectionName].Parameters[SanityTest.ConfigurationAddressParameterName].Value; 
                }

                return Task.Factory.StartNew<string>(() => address);
            }

            public Task CloseAsync(CancellationToken cancellationToken)
            {
                return Task.Factory.StartNew(() =>
                    {
                        Debug.WriteLine("OnClose");
                        this.WriteEventToGlobalFile(SanityTest.CloseAsyncMarker);
                    });
            }

            public void Abort()
            {
                Debug.WriteLine("Abort");
                this.WriteEventToGlobalFile(SanityTest.AbortMarker);
            }

            public Threading.Tasks.Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
            {
                return Task.Factory.StartNew<IReplicator>(() =>
                    {
                        Debug.WriteLine("OnOpen");
                        this.WriteEventToGlobalFile(SanityTest.OpenAsyncMarker);
                        return partition.CreateReplicator(this, null);
                    });
            }

            public long GetLastCommittedSequenceNumber()
            {
                return -1;
            }

            public IOperationDataStream GetCopyContext()
            {
                return null;
            }

            public IOperationDataStream GetCopyState(long sequenceNumber, IOperationDataStream copyContext)
            {
                return new AsyncEnumOpData();
            }

            public Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
            {
                return Task.Factory.StartNew(() => false);
            }

            public Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken)
            {
                return Task.Factory.StartNew(() => { ;});
            }

            class AsyncEnumOpData : IOperationDataStream
            {
                public Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
                {
                    return Task.Factory.StartNew<OperationData>(() => null);
                }
            }
        };

        #endregion

        public static ServiceDeployer GetService(int replicaSetSize)
        {
            return new ServiceDeployer()
            {
                CodePackageFiles = new List<string>(SanityTest.GetAllCodePackageBinaries()),
                InstanceCount = 0,
                ServiceTypeImplementation = typeof(StatefulSanityTest.MyApplication),
                ServiceTypeName = "SanityTestStatefulService",
                MinReplicaSetSize = replicaSetSize,
                TargetReplicaSetSize = replicaSetSize
            };
        }
    }
}