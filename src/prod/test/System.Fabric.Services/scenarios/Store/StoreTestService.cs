// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Services.Scenarios.Store.Test
{
    using System;
    using Collections.Generic;
    using Diagnostics;
    using Fabric;
    using Description;
    using IO;
    using Linq;
    using Net;
    using Text;
    using Threading;
    using Threading.Tasks;
    using Globalization;

    public sealed class StoreTestService : KeyValueStoreReplica
    {
        public const string CommandTestZeroLengthValue = "Test.ZeroLengthValue";
        public const string CommandTestKeyNotFound = "Test.KeyNotFound";
        public const string CommandTestContains = "Test.Contains";
        public const string CommandTestUpdateReplicatorSettings1 = "Test.UpdateReplicatorSettings1";
        public const string CommandTestUpdateReplicatorSettings2 = "Test.UpdateReplicatorSettings2";
        public const string CommandTestOutofBoundReplicatorSettings = "Test.OutofBoundReplicatorSettings";
        public const string CommandReportFault = "ReportFault";
        public const string CommandTestBackupAsyncFull = "Test.BackupAsync.Full";
        public const string CommandTestBackupAsyncIncremental = "Test.BackupAsync.Incremental";
        public const string CommandTestBackupAsyncTruncateLogsOnly = "Test.BackupAsync.TruncateLogsOnly";
        public const string CommandSetTrueReturnFromPostBackupHandler = "PostBackupHandler.ReturnTrue";
        public const string CommandSetFalseReturnFromPostBackupHandler = "PostBackupHandler.ReturnFalse";
        public const string CommandTestBackupAsyncIncrementalAfterFalseReturn = "Test.BackupAsync.Incremental.AfterFalseReturn";
        public const string CommandTestBackupAsyncIncrementalAfterTruncateLogsOnly = "Test.BackupAsync.Incremental.AfterTruncateLogsOnly";
        public const string CommandTestBackupDirectoryNotEmptyException = "Test.BackupDirectoryNotEmptyException";
        public const string CommandTestBackupInProgressException = "Test.BackupInProgressException";
        public const string CommandTestAddOrUpdate = "Test.AddOrUpdate";
        public const string CommandTestVerifyTestData = "Test.VerifyTestData";
        public const string CommandTestTryUpdatePerf = "Test.TryUpdatePerf";
        public const string CommandTestLastModifiedOnPrimary = "Test.LastModifiedOnPrimary";
        public const string CommandTestCommitNotCancellable = "Test.TestCommitNotCancellable";
        public const string CommandTestEnsurePrimary = "Test.EnsurePrimary";
        public const string CommandTestSettings = "Test.Settings";

        private readonly object syncLock = new object();

        private HttpListener listener;
        private int data = Constants.ServiceCounterInitialValue;

        private string logFile;

        private IStatefulServicePartition partition;
        private ServiceInitializationParameters initializationParameters;
        private Guid partitionId;
        private long replicaId;

        private ManualResetEvent closeEvent = new ManualResetEvent(false);

        private bool postBackupHandlerReturnValue = true;

        private readonly ManualResetEvent backupInProgressTestingWaiter = new ManualResetEvent(false);

        #region IStatefulServiceReplica

        public ReplicaRole ReplicaRole { get; private set; }

        public StoreTestService()
            : base(
                "StoreTestService",
                CreateEseSettings(),
                CreateReplicatorSettings(),
                CreateKvsSettings())
        {
        }

        private static ReplicatorSettings CreateReplicatorSettings()
        {
            return new ReplicatorSettings()
            {
                MaxReplicationMessageSize = 1024*1024*200
            };
        }

        private static LocalEseStoreSettings CreateEseSettings()
        {
            return new LocalEseStoreSettings 
            { 
                LogFileSizeInKB = 10240,
                LogBufferSizeInKB = 2048,
                MaxCursors = 32768,
                MaxVerPages = 65536,
                MaxAsyncCommitDelay = TimeSpan.FromMilliseconds(213),
                EnableIncrementalBackup = true,
                MaxCacheSizeInMB = 512,
                MaxDefragFrequencyInMinutes = 1,
                DefragThresholdInMB = 2,
                DatabasePageSizeInKB = 32,
                CompactionThresholdInMB = 1024,
                IntrinsicValueThresholdInBytes = 2048,
            };
        }

        private static KeyValueStoreReplicaSettings CreateKvsSettings()
        {
            return new KeyValueStoreReplicaSettings
            {
                TransactionDrainTimeout = TimeSpan.FromSeconds(42),
                EnableCopyNotificationPrefetch = false,
                SecondaryNotificationMode = SecondaryNotificationMode.NonBlockingQuorumAcked,
                FullCopyMode = FullCopyMode.Rebuild,
            };
        }

        protected override void OnInitialize(StatefulServiceInitializationParameters initializationParameters)
        {
            this.initializationParameters = initializationParameters;
            this.partitionId = initializationParameters.PartitionId;
            this.replicaId = initializationParameters.ReplicaId;
            base.OnInitialize(initializationParameters);
        }

        protected override Task OnOpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            string folder = this.initializationParameters.CodePackageActivationContext.GetConfigurationPackageObject(Constants.ConfigurationPackageName).Settings.Sections[Constants.ConfigurationSectionName].Parameters[Constants.ConfigurationSettingName].Value;
            this.logFile = Path.Combine(folder, string.Format("{0}_{1}.out", partition.PartitionInfo.Id, this.replicaId));

            IList<string> names = this.initializationParameters.CodePackageActivationContext.GetCodePackageNames();
            foreach (string name in names)
            {
                CodePackageDescription codePackage = this.initializationParameters.CodePackageActivationContext.GetCodePackageObject(name).Description;
                this.Log("CodePackage name {0}", codePackage.Name);
            }

            this.Log("OpenAsync");
            this.partition = partition;

            string uri = string.Format("http://localhost:{0}/", this.initializationParameters.CodePackageActivationContext.GetEndpoint(Constants.EndpointName).Port);

            this.Log("Listening at: {0}", uri);

            this.listener = new HttpListener();
            this.listener.Prefixes.Add(uri);
            this.listener.Start();

            this.ProcessRequestLoop();

            return base.OnOpenAsync(openMode, partition, cancellationToken);
        }

        protected override Task<string> OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew<string>(() =>
            {
                this.Log("ReplicaRoleChangeAsync from {0} to {1}", this.ReplicaRole, newRole);

                ReplicaRole oldRole = this.ReplicaRole;
                this.ReplicaRole = newRole;

                return new Uri(this.listener.Prefixes.First()).ToString();
            });
        }

        protected override Task OnCloseAsync(CancellationToken cancellationToken)
        {
            this.Log("CloseAsync");
            this.closeEvent.Set();
            this.listener.Stop();
            return base.OnCloseAsync(cancellationToken);
        }

        protected override void OnAbort()
        {
            this.listener.Abort();
            base.OnAbort();
        }

        protected override void OnDatalossReported(EventArgs args)
        {
            this.Log("OnDataLossAsync");
            base.OnDatalossReported(args);
        }

        protected override void OnCopyComplete(KeyValueStoreEnumerator enumerator)
        {
            try
            {
                this.Log("OnCopyComplete");

                // TODO: TStore currently does not support Reset().
                //       See TSChangeHandler::StoreEnumerator
                //

                //this.EnumerateMetadataOnCopyComplete(enumerator);

                //this.EnumerateOnCopyComplete(enumerator);
            }
            catch (Exception e)
            {

                this.Log("OnCopyComplete: {0}", e);
            }
        }

        private void EnumerateMetadataOnCopyComplete(KeyValueStoreEnumerator enumerator)
        {
            int totalItemCount = 0;

            var metadataEnumerator = enumerator.EnumerateMetadata(string.Empty);
            while (metadataEnumerator.MoveNext())
            {
                ++totalItemCount;

                this.Log(
                    "EnumerateMetadata: key = '{0}' size = {1} bytes lsn = '{2}'",
                    metadataEnumerator.Current.Key,
                    metadataEnumerator.Current.ValueSizeInBytes,
                    metadataEnumerator.Current.SequenceNumber);

                int size = 0;
                if (int.TryParse(metadataEnumerator.Current.Key, out size))
                {
                    if (size != metadataEnumerator.Current.ValueSizeInBytes)
                    {
                        throw new InvalidOperationException(string.Format("EnumerateMetadata: {0} != {1}", size, metadataEnumerator.Current.ValueSizeInBytes));
                    }

                    this.Log("Verified size = {0}", size);
                }
                else if (metadataEnumerator.Current.ValueSizeInBytes <= 0)
                {
                    throw new InvalidOperationException(string.Format(
                        "EnumerateMetadata: {0} <= 0", 
                        metadataEnumerator.Current.ValueSizeInBytes));
                }

                var metadata = metadataEnumerator.Current;

                this.Log(
                    "EnumerateMetadata-cached: key = '{0}' size = {1} bytes lsn = '{2}'",
                    metadata.Key,
                    metadata.ValueSizeInBytes,
                    metadata.SequenceNumber);

                if (int.TryParse(metadata.Key, out size))
                {
                    if (size != metadata.ValueSizeInBytes)
                    {
                        throw new InvalidOperationException(string.Format("EnumerateMetadata-cached: {0} != {1}", size, metadata.ValueSizeInBytes));
                    }

                    this.Log("Verified size = {0}", size);
                }
                else if (metadata.ValueSizeInBytes <= 0)
                {
                    throw new InvalidOperationException(string.Format(
                        "EnumerateMetadata: {0} <= 0", 
                        metadata.ValueSizeInBytes));
                }

                if (metadataEnumerator.Current.ValueSizeInBytes != metadata.ValueSizeInBytes)
                {
                    throw new InvalidOperationException(string.Format(
                        "EnumerateMetadata: metadata({0}), metadata-cached({1}) mismatch",
                        metadataEnumerator.Current.ValueSizeInBytes,
                        metadata.ValueSizeInBytes));
                }
            }

            // Skip prefix test when there are no items
            //
            if (totalItemCount == 0)
            {
                return;
            }

            for (var ix=1; ix<=FabricStoreTest.PrefixItemCount; ++ix)
            {
                var itemCount = 0;
                var prefix = ix.ToString("D2");
                var prefixEnumerator = enumerator.EnumerateMetadata(prefix);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "EnumerateMetadata-Prefix('{0}'): key = '{1}' size = {2} bytes lsn = '{3}'",
                        prefix,
                        prefixEnumerator.Current.Key,
                        prefixEnumerator.Current.ValueSizeInBytes,
                        prefixEnumerator.Current.SequenceNumber);

                    ++itemCount;
                }

                // Notification layer should filter out non-matching prefixes for the service.
                // Symmetric behavior with KVS enumerations.
                //
                if (itemCount != 1)
                {
                    throw new InvalidOperationException(string.Format("EnumerateMetadata-Prefix: {0} != 1", itemCount));
                }
            }

            {
                var itemCount = 0;
                var prefix = "0";
                var prefixEnumerator = enumerator.EnumerateMetadata(prefix);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "EnumerateMetadata-Prefix('{0}'): key = '{1}' size = {2} bytes lsn = '{3}'",
                        prefix,
                        prefixEnumerator.Current.Key,
                        prefixEnumerator.Current.ValueSizeInBytes,
                        prefixEnumerator.Current.SequenceNumber);

                    ++itemCount;
                }

                // Notification layer should filter out non-matching prefixes for the service.
                // Symmetric behavior with KVS enumerations.
                //
                if (itemCount != 9)
                {
                    throw new InvalidOperationException(string.Format("EnumerateMetadata-Prefix: {0} != 9", itemCount));
                }
            }

            // Non-strict prefix matching allows non-atomic paging across multiple enumerations
            {
                var itemCount = 0;
                var prefix = "00";
                var prefixEnumerator = enumerator.EnumerateMetadata(prefix, false);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "EnumerateMetadata-NonStrict-Prefix('{0}'): key = '{1}' size = {2} bytes lsn = '{3}'",
                        prefix,
                        prefixEnumerator.Current.Key,
                        prefixEnumerator.Current.ValueSizeInBytes,
                        prefixEnumerator.Current.SequenceNumber);

                    ++itemCount;
                }

                if (itemCount != 10)
                {
                    throw new InvalidOperationException(string.Format("EnumerateMetadata-NonStrict-Prefix: {0} != 10", itemCount));
                }
            }

            {
                var itemCount = 0;
                var prefix = "05b";
                var prefixEnumerator = enumerator.EnumerateMetadata(prefix, false);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "EnumerateMetadata-NonStrict-Prefix('{0}'): key = '{1}' size = {2} bytes lsn = '{3}'",
                        prefix,
                        prefixEnumerator.Current.Key,
                        prefixEnumerator.Current.ValueSizeInBytes,
                        prefixEnumerator.Current.SequenceNumber);

                    ++itemCount;
                }

                if (itemCount != 5)
                {
                    throw new InvalidOperationException(string.Format("EnumerateMetadata-NonStrict-Prefix: {0} != 5", itemCount));
                }
            }

            {
                var itemCount = 0;
                var prefix = "101";
                var prefixEnumerator = enumerator.EnumerateMetadata(prefix, false);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "EnumerateMetadata-NonStrict-Prefix('{0}'): key = '{1}' size = {2} bytes lsn = '{3}'",
                        prefix,
                        prefixEnumerator.Current.Key,
                        prefixEnumerator.Current.ValueSizeInBytes,
                        prefixEnumerator.Current.SequenceNumber);

                    ++itemCount;
                }

                if (itemCount != 0)
                {
                    throw new InvalidOperationException(string.Format("EnumerateMetadata-NonStrict-Prefix: {0} != 0", itemCount));
                }
            }
        }

        private void EnumerateOnCopyComplete(KeyValueStoreEnumerator enumerator)
        {
            int totalItemCount = 0;

            var itemEnumerator = enumerator.Enumerate(string.Empty);
            while (itemEnumerator.MoveNext())
            {
                ++totalItemCount;

                this.Log(
                    "Enumerate: key = '{0}' value = {1}/{2} bytes lsn = '{3}'",
                    itemEnumerator.Current.Metadata.Key,
                    itemEnumerator.Current.Metadata.ValueSizeInBytes,
                    itemEnumerator.Current.Value.Length,
                    itemEnumerator.Current.Metadata.SequenceNumber);
                
                int size = 0;
                if (int.TryParse(itemEnumerator.Current.Metadata.Key, out size))
                {
                    if (size != itemEnumerator.Current.Metadata.ValueSizeInBytes)
                    {
                        throw new InvalidOperationException(string.Format("Enumerate: {0} != {1}", size, itemEnumerator.Current.Metadata.ValueSizeInBytes));
                    }

                    if (size != itemEnumerator.Current.Value.Length)
                    {
                        throw new InvalidOperationException(string.Format("Enumerate(value): {0} != {1}", size, itemEnumerator.Current.Value.Length));
                    }

                    this.Log("Verified size = {0}", size);
                }
                else if (itemEnumerator.Current.Metadata.ValueSizeInBytes <= 0 || itemEnumerator.Current.Value.Length <= 0) 
                {
                    throw new InvalidOperationException(string.Format(
                        "Enumerate: metadata({0}) or value({1}) <= 0", 
                        itemEnumerator.Current.Metadata.ValueSizeInBytes, 
                        itemEnumerator.Current.Value.Length));
                }

                var item = itemEnumerator.Current;

                this.Log(
                    "Enumerate-cached: key = '{0}' value = {1}/{2} bytes lsn = '{3}'",
                    item.Metadata.Key,
                    item.Metadata.ValueSizeInBytes,
                    item.Value.Length,
                    item.Metadata.SequenceNumber);

                if (int.TryParse(item.Metadata.Key, out size))
                {
                    if (size != item.Metadata.ValueSizeInBytes)
                    {
                        throw new InvalidOperationException(string.Format("Enumerate-cached: {0} != {1}", size, item.Metadata.ValueSizeInBytes));
                    }

                    if (size != item.Value.Length)
                    {
                        throw new InvalidOperationException(string.Format("Enumerate-cached(value): {0} != {1}", size, item.Value.Length));
                    }

                    this.Log("Verified size = {0}", size);
                }
                else if (item.Metadata.ValueSizeInBytes <= 0 || item.Value.Length <= 0)
                {
                    throw new InvalidOperationException(string.Format(
                        "Enumerate-cached: metadata({0}) or value({1}) <= 0", 
                        item.Metadata.ValueSizeInBytes, 
                        item.Value.Length));
                }

                if (itemEnumerator.Current.Metadata.ValueSizeInBytes != itemEnumerator.Current.Value.Length ||
                    itemEnumerator.Current.Metadata.ValueSizeInBytes != item.Metadata.ValueSizeInBytes ||
                    itemEnumerator.Current.Metadata.ValueSizeInBytes != item.Value.Length)
                {
                    throw new InvalidOperationException(string.Format(
                        "Enumerate: metadata({0}), value({1}), metadata-cached({2}), value-cached({3}) mismatch",
                        itemEnumerator.Current.Metadata.ValueSizeInBytes,
                        itemEnumerator.Current.Value.Length,
                        item.Metadata.ValueSizeInBytes, 
                        item.Value.Length));
                }
            }

            // Skip prefix test when there are no items
            //
            if (totalItemCount == 0)
            {
                return;
            }

            IList<IEnumerator<KeyValueStoreItem>> prefixEnumerators = new List<IEnumerator<KeyValueStoreItem>>();

            for (var ix=1; ix<=FabricStoreTest.PrefixItemCount; ++ix)
            {
                var itemCount = 0;
                var prefix = ix.ToString("D2");
                var prefixEnumerator = enumerator.Enumerate(prefix);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "Enumerate-Prefix('{0}'): key = '{1}' value = {2}/{3} bytes lsn = '{4}'",
                        prefix,
                        prefixEnumerator.Current.Metadata.Key,
                        prefixEnumerator.Current.Metadata.ValueSizeInBytes,
                        prefixEnumerator.Current.Value.Length,
                        prefixEnumerator.Current.Metadata.SequenceNumber);

                    ++itemCount;
                }

                prefixEnumerators.Add(prefixEnumerator);

                // Notification layer should filter out non-matching prefixes for the service.
                // Symmetric behavior with KVS enumerations.
                //
                if (itemCount != 1)
                {
                    throw new InvalidOperationException(string.Format("Enumerate-Prefix: {0} != 1", itemCount));
                }
            }

            {
                var itemCount = 0;
                var prefix = "0";
                var prefixEnumerator = enumerator.Enumerate(prefix);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "Enumerate-Prefix('{0}'): key = '{1}' value = {2}/{3} bytes lsn = '{4}'",
                        prefix,
                        prefixEnumerator.Current.Metadata.Key,
                        prefixEnumerator.Current.Metadata.ValueSizeInBytes,
                        prefixEnumerator.Current.Value.Length,
                        prefixEnumerator.Current.Metadata.SequenceNumber);

                    ++itemCount;
                }

                prefixEnumerators.Add(prefixEnumerator);

                // Notification layer should filter out non-matching prefixes for the service.
                // Symmetric behavior with KVS enumerations.
                //
                if (itemCount != 9)
                {
                    throw new InvalidOperationException(string.Format("Enumerate-Prefix: {0} != 9", itemCount));
                }
            }

            // Non-strict prefix matching allows non-atomic paging across multiple enumerations
            {
                var itemCount = 0;
                var prefix = "00";
                var prefixEnumerator = enumerator.Enumerate(prefix, false);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "Enumerate-NonStrict-Prefix('{0}'): key = '{1}' size = {2}/{3} bytes lsn = '{4}'",
                        prefix,
                        prefixEnumerator.Current.Metadata.Key,
                        prefixEnumerator.Current.Metadata.ValueSizeInBytes,
                        prefixEnumerator.Current.Value.Length,
                        prefixEnumerator.Current.Metadata.SequenceNumber);

                    ++itemCount;
                }

                if (itemCount != 10)
                {
                    throw new InvalidOperationException(string.Format("Enumerate-NonStrict-Prefix: {0} != 10", itemCount));
                }
            }

            {
                var itemCount = 0;
                var prefix = "05b";
                var prefixEnumerator = enumerator.Enumerate(prefix, false);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "Enumerate-NonStrict-Prefix('{0}'): key = '{1}' size = {2}/{3} bytes lsn = '{4}'",
                        prefix,
                        prefixEnumerator.Current.Metadata.Key,
                        prefixEnumerator.Current.Metadata.ValueSizeInBytes,
                        prefixEnumerator.Current.Value.Length,
                        prefixEnumerator.Current.Metadata.SequenceNumber);

                    ++itemCount;
                }

                if (itemCount != 5)
                {
                    throw new InvalidOperationException(string.Format("Enumerate-NonStrict-Prefix: {0} != 5", itemCount));
                }
            }

            {
                var itemCount = 0;
                var prefix = "101";
                var prefixEnumerator = enumerator.Enumerate(prefix, false);

                while (prefixEnumerator.MoveNext())
                {
                    this.Log(
                        "Enumerate-NonStrict-Prefix('{0}'): key = '{1}' size = {2}/{3} bytes lsn = '{4}'",
                        prefix,
                        prefixEnumerator.Current.Metadata.Key,
                        prefixEnumerator.Current.Metadata.ValueSizeInBytes,
                        prefixEnumerator.Current.Value.Length,
                        prefixEnumerator.Current.Metadata.SequenceNumber);

                    ++itemCount;
                }

                if (itemCount != 0)
                {
                    throw new InvalidOperationException(string.Format("Enumerate-NonStrict-Prefix: {0} != 0", itemCount));
                }
            }

            // The service is expected to dispose of these enumerators. However, we must function correctly
            // even if service does not and there is an ESE bug where JetCloseTable can AV if called
            // on a table that is already closed. Intentionally dispose enumerators outside the OnCopyComplete() 
            // callback to simulate GC after the callback returns.
            //
            Task.Delay(TimeSpan.FromSeconds(FabricStoreTest.PrefixEnumeratorDisposeDelayInSeconds)).ContinueWith((continuation) =>
            {
                this.Log("Disposing prefix enumerators");

                foreach (var prefixEnumerator in prefixEnumerators)
                {
                    prefixEnumerator.Dispose();
                }
            });
        }

        protected override void OnReplicationOperation(IEnumerator<KeyValueStoreNotification> enumerator)
        {
            try
            {
                this.ProcessReplicationNotification(enumerator);
            }
            catch (Exception e)
            {

                this.Log("OnReplicationOperation: {0}", e);
            }
        }

        private void ProcessReplicationNotification(IEnumerator<KeyValueStoreNotification> enumerator)
        {
            while (enumerator.MoveNext())
            {
                this.Log(
                    "OnReplicationOperation: key = '{0}' value = {1}/{2} bytes lsn = '{3}' delete = {4}",
                    enumerator.Current.Metadata.Key,
                    enumerator.Current.Metadata.ValueSizeInBytes,
                    enumerator.Current.Value.Length,
                    enumerator.Current.Metadata.SequenceNumber,
                    enumerator.Current.IsDelete);

                int size = 0;
                if (int.TryParse(enumerator.Current.Metadata.Key, out size))
                {
                    if (size != enumerator.Current.Metadata.ValueSizeInBytes)
                    {
                        throw new InvalidOperationException(string.Format("Replication: {0} != {1}", size, enumerator.Current.Metadata.ValueSizeInBytes));
                    }

                    if (size != enumerator.Current.Value.Length)
                    {
                        throw new InvalidOperationException(string.Format("Replication(value): {0} != {1}", size, enumerator.Current.Value.Length));
                    }

                    this.Log("Verified size = {0}", size);
                }
                else if (!enumerator.Current.IsDelete)
                {
                    if (enumerator.Current.Metadata.ValueSizeInBytes <= 0 || enumerator.Current.Value.Length <= 0) 
                    {
                        throw new InvalidOperationException(string.Format(
                            "Replication: metadata({0}) or value({1}) <= 0", 
                            enumerator.Current.Metadata.ValueSizeInBytes, 
                            enumerator.Current.Value.Length));
                    }
                }
                
                var item = enumerator.Current;

                this.Log(
                    "OnReplicationOperation-cached: key = '{0}' value = {1}/{2} bytes lsn = '{3}' delete = {4}",
                    item.Metadata.Key,
                    item.Metadata.ValueSizeInBytes,
                    item.Value.Length,
                    item.Metadata.SequenceNumber,
                    item.IsDelete);
                
                if (int.TryParse(item.Metadata.Key, out size))
                {
                    if (size != item.Metadata.ValueSizeInBytes)
                    {
                        throw new InvalidOperationException(string.Format("Replication-cached: {0} != {1}", size, item.Metadata.ValueSizeInBytes));
                    }

                    if (size != item.Value.Length)
                    {
                        throw new InvalidOperationException(string.Format("Replication-cached(value): {0} != {1}", size, item.Value.Length));
                    }

                    this.Log("Verified size = {0}", size);
                }
                else if (!item.IsDelete)
                {
                    if (item.Metadata.ValueSizeInBytes <= 0 || item.Value.Length <= 0)
                    {
                        throw new InvalidOperationException(string.Format(
                            "Replication-cached: metadata({0}) or value({1}) <= 0", 
                            item.Metadata.ValueSizeInBytes, 
                            item.Value.Length));
                    }
                }

                if (enumerator.Current.Metadata.ValueSizeInBytes != enumerator.Current.Value.Length ||
                    enumerator.Current.Metadata.ValueSizeInBytes != item.Metadata.ValueSizeInBytes ||
                    enumerator.Current.Metadata.ValueSizeInBytes != item.Value.Length)
                {
                    throw new InvalidOperationException(string.Format(
                        "Replication: metadata({0}), value({1}), metadata-cached({2}), value-cached({3}) mismatch",
                        enumerator.Current.Metadata.ValueSizeInBytes,
                        enumerator.Current.Value.Length,
                        item.Metadata.ValueSizeInBytes, 
                        item.Value.Length));
                }
            }
        }
    
        #endregion

        private void ProcessRequestLoop()
        {
            Task.Factory.FromAsync<HttpListenerContext>(this.listener.BeginGetContext, this.listener.EndGetContext, null).ContinueWith((t) =>
            {
                if (t.IsFaulted)
                {
                    this.Log("Exception when reading request: {0}", t.Exception.InnerException);
                    return;
                }

                // continue async processing
                this.ProcessRequestLoop();
                this.HandleRequest(t.Result);
            });
        }

        private void HandleRequest(HttpListenerContext listenerContext)
        {
            string content = string.Empty;
            try 
            {
                if (listenerContext.Request.HttpMethod == "POST")
                {
                    using (StreamReader sr = new StreamReader(listenerContext.Request.InputStream))
                    {
                        content = sr.ReadToEnd();
                    }

                    if (string.Equals(content, CommandTestSettings, StringComparison.OrdinalIgnoreCase))
                    {
                        this.TestSettings(listenerContext);
                    }
                    else if(string.Equals(content, CommandTestEnsurePrimary, StringComparison.OrdinalIgnoreCase))
                    {
                        this.TestEnsurePrimary(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestLastModifiedOnPrimary, StringComparison.OrdinalIgnoreCase))
                    {
                        this.TestLastModifiedOnPrimary(listenerContext);
                    }
                    else if(string.Equals(content, CommandTestCommitNotCancellable, StringComparison.OrdinalIgnoreCase))
                    {
                        this.TestCommitNotCancellable(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestZeroLengthValue, StringComparison.OrdinalIgnoreCase))
                    {
                        TestZeroLengthValue(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestKeyNotFound, StringComparison.OrdinalIgnoreCase))
                    {
                        TestKeyNotFound(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestContains, StringComparison.OrdinalIgnoreCase))
                    {
                        TestContains(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestUpdateReplicatorSettings1, StringComparison.OrdinalIgnoreCase))
                    {
                        TestUpdateReplicatorSettings1(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestUpdateReplicatorSettings2, StringComparison.OrdinalIgnoreCase))
                    {
                        TestUpdateReplicatorSettings2(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestOutofBoundReplicatorSettings, StringComparison.OrdinalIgnoreCase))
                    {
                        TestOutOfBoundReplicatorSettings(listenerContext);
                    }
                    else if (string.Equals(content, CommandReportFault, StringComparison.OrdinalIgnoreCase))
                    {
                        DoReportFault(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestBackupAsyncFull, StringComparison.OrdinalIgnoreCase))
                    {
                        TestBasicBackup(listenerContext, StoreBackupOption.Full);
                    }
                    else if (string.Equals(content, CommandTestBackupAsyncIncremental, StringComparison.OrdinalIgnoreCase))
                    {
                        TestBasicBackup(listenerContext, StoreBackupOption.Incremental);
                    }
                    else if (string.Equals(content, CommandTestBackupAsyncTruncateLogsOnly, StringComparison.OrdinalIgnoreCase))
                    {
                        TestBasicBackup(listenerContext, StoreBackupOption.TruncateLogsOnly);
                    }
                    else if (string.Equals(content, CommandSetTrueReturnFromPostBackupHandler, StringComparison.OrdinalIgnoreCase))
                    {
                        UpdatePostBackupHandlerValue(listenerContext, true);
                    }
                    else if (string.Equals(content, CommandSetFalseReturnFromPostBackupHandler, StringComparison.OrdinalIgnoreCase))
                    {
                        UpdatePostBackupHandlerValue(listenerContext, false);
                    }
                    else if (string.Equals(content, CommandTestBackupAsyncIncrementalAfterFalseReturn, StringComparison.OrdinalIgnoreCase))
                    {
                        TestIncrementalBackupAfterFalseReturnFromPostBackupHandler(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestBackupAsyncIncrementalAfterTruncateLogsOnly, StringComparison.OrdinalIgnoreCase))
                    {
                        TestIncrementalBackupAfterTruncateLogsOnly(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestBackupDirectoryNotEmptyException, StringComparison.OrdinalIgnoreCase))
                    {
                        TestBackupDirectoryNotEmptyException(listenerContext);
                    }
                    else if (string.Equals(content, CommandTestBackupInProgressException, StringComparison.OrdinalIgnoreCase))
                    {
                        TestBackupInProgressException(listenerContext).Wait();
                    }         
                    else if (string.Equals(content, CommandTestAddOrUpdate, StringComparison.OrdinalIgnoreCase))
                    {
                        AddOrUpdate(listenerContext);
                    }         
                    else if (string.Equals(content, CommandTestTryUpdatePerf, StringComparison.OrdinalIgnoreCase))
                    {
                        TryUpdatePerf(listenerContext);
                    }         
                    else if (string.Equals(content, CommandTestVerifyTestData, StringComparison.OrdinalIgnoreCase))
                    {
                        VerifyTestData(listenerContext);
                    }         
                    else
                    {
                        AddTestData(listenerContext);
                    }
                }
                else  // GET request
                {
                    ReadTestData(listenerContext);    
                }
            }
            catch (Exception ex)
            {
                HandleException(listenerContext, content, ex);
            }
        }

        private void AddTestData(HttpListenerContext listenerContext)
        {
            try
            {
                int tempData = this.data + 1;
                using (var txn = this.CreateTransaction())
                {
                    var bytes = new byte[tempData];
                    for (var ix=0; ix<bytes.Length; ++ix)
                    {
                        bytes[ix] = (byte)ix;
                    }

                    this.Add(txn, tempData.ToString("D2"), bytes);

                    if (!this.TryUpdate(txn, tempData.ToString("D2"), bytes))
                    {
                        throw new InvalidOperationException(string.Format("TryUpdate({0}) failed", tempData));
                    }

                    var commitTask = txn.CommitAsync();
                    commitTask.Wait();
                }

                this.data = tempData;

                this.Log("success: {0}", tempData);
                byte[] resp = Encoding.UTF8.GetBytes(this.data.ToString());
                listenerContext.Response.StatusCode = (int)HttpStatusCode.OK;
                listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
                listenerContext.Response.Close();
            }
            catch (Exception ex)
            {
                string error = string.Format("Replication failed with {0}", ex);
                this.Log(error);

                byte[] resp = Encoding.UTF8.GetBytes(error);
                listenerContext.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
                listenerContext.Response.Close();
            }
        }

        // For FabricStoreBackupTest1, FabricStoreBackupTest2
        //
        private void ReadTestData(HttpListenerContext listenerContext)
        {
            this.ReadTestData(listenerContext, false);
        }

        private void VerifyTestData(HttpListenerContext listenerContext)
        {
            this.ReadTestData(listenerContext, true);
        }

        private void ReadTestData(HttpListenerContext listenerContext, bool verifyCounts)
        {
            try
            {
                int lastKey = 0;

                using (var txn = this.CreateTransaction())
                {
                    using (var enumeration = this.EnumerateMetadata(txn))
                    {
                        var itemCount = 0;
                        while (enumeration.MoveNext())
                        {
                            ++itemCount;

                            var metadata = enumeration.Current;
                            var key = int.Parse(metadata.Key);

                            this.Log(
                                "Metadata: key={0} LSN={1} size={2}", 
                                metadata.Key, 
                                metadata.SequenceNumber, 
                                metadata.ValueSizeInBytes);

                            if (metadata.ValueSizeInBytes != key)
                            {
                                throw new InvalidOperationException(string.Format(
                                    "Metadata: {0} != {1}",
                                    metadata.ValueSizeInBytes,
                                    key));
                            }
                        }

                        if (verifyCounts && itemCount != 10)
                        {
                            throw new InvalidOperationException(string.Format(
                                "Metadata: count({0}) != 10",
                                itemCount));
                        }
                    }

                    using (var enumeration = this.Enumerate(txn))
                    {
                        var itemCount = 0;
                        while (enumeration.MoveNext())
                        {
                            ++itemCount;

                            var item = enumeration.Current;
                            var key = int.Parse(item.Metadata.Key);

                            this.Log(
                                "Item: key={0} LSN={1} size={2} bytes={3}", 
                                item.Metadata.Key, 
                                item.Metadata.SequenceNumber, 
                                item.Metadata.ValueSizeInBytes,
                                item.Value.Length);

                            if (item.Metadata.ValueSizeInBytes != item.Value.Length)
                            {
                                throw new InvalidOperationException(string.Format(
                                    "Item: {0} != {1}",
                                    item.Metadata.ValueSizeInBytes,
                                    item.Value.Length));
                            }

                            if (item.Metadata.ValueSizeInBytes != key)
                            {
                                throw new InvalidOperationException(string.Format(
                                    "Item(key): {0} != {1}",
                                    item.Metadata.ValueSizeInBytes,
                                    key));
                            }

                            lastKey = key;
                        }

                        if (verifyCounts && itemCount != 10)
                        {
                            throw new InvalidOperationException(string.Format(
                                "Item: count({0}) != 10",
                                itemCount));
                        }
                    }

                    // Non-strict prefix
                    //
                    using (var enumeration = this.EnumerateMetadata(txn, "04.5x", false))
                    {
                        var itemCount = 0;
                        while (enumeration.MoveNext())
                        {
                            ++itemCount;

                            var metadata = enumeration.Current;
                            var key = int.Parse(metadata.Key);

                            this.Log(
                                "Metadata, Non-strict Prefix: key={0} LSN={1} size={2}", 
                                metadata.Key, 
                                metadata.SequenceNumber, 
                                metadata.ValueSizeInBytes);

                            if (metadata.ValueSizeInBytes != key)
                            {
                                throw new InvalidOperationException(string.Format(
                                    "Metadata, Non-strict Prefix: {0} != {1}",
                                    metadata.ValueSizeInBytes,
                                    key));
                            }
                        }

                        if (verifyCounts && itemCount != 6)
                        {
                            throw new InvalidOperationException(string.Format(
                                "Metadata, Non-strict Prefix: count({0}) != 6",
                                itemCount));
                        }
                    }

                    using (var enumeration = this.Enumerate(txn, "06prefix", false))
                    {
                        var itemCount = 0;
                        while (enumeration.MoveNext())
                        {
                            ++itemCount;

                            var item = enumeration.Current;
                            var key = int.Parse(item.Metadata.Key);

                            this.Log(
                                "Item, Non-strict Prefix: key={0} LSN={1} size={2} bytes={3}", 
                                item.Metadata.Key, 
                                item.Metadata.SequenceNumber, 
                                item.Metadata.ValueSizeInBytes,
                                item.Value.Length);

                            if (item.Metadata.ValueSizeInBytes != item.Value.Length)
                            {
                                throw new InvalidOperationException(string.Format(
                                    "Item, Non-strict Prefix: {0} != {1}",
                                    item.Metadata.ValueSizeInBytes,
                                    item.Value.Length));
                            }

                            if (item.Metadata.ValueSizeInBytes != key)
                            {
                                throw new InvalidOperationException(string.Format(
                                    "Item(key), Non-strict Prefix: {0} != {1}",
                                    item.Metadata.ValueSizeInBytes,
                                    key));
                            }

                            lastKey = key;
                        }

                        if (verifyCounts && itemCount != 4)
                        {
                            throw new InvalidOperationException(string.Format(
                                "Metadata, Non-strict Prefix: count({0}) != 4",
                                itemCount));
                        }
                    }
                }

                byte[] resp = Encoding.UTF8.GetBytes(lastKey.ToString());
                listenerContext.Response.StatusCode = (int)HttpStatusCode.OK;
                listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
                listenerContext.Response.Close();
            }
            catch (Exception ex)
            {
                string error = string.Format("ReadTestData failed with {0}", ex);
                this.Log(error);

                byte[] resp = Encoding.UTF8.GetBytes(error);
                listenerContext.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
                listenerContext.Response.Close();
            }
        }

        private void AddOrUpdate(HttpListenerContext listenerContext)
        {
            try
            {
                string addOrUpdateKey = "AddOrUpdateTestKey"; 

                using (var txn = this.CreateTransaction())
                {
                    if (this.TryUpdate(txn, addOrUpdateKey, new byte[] { 0 }))
                    {
                        throw new InvalidOperationException(string.Format("Should have added {0}", addOrUpdateKey));
                    }
                    else
                    {
                        this.Add(txn, addOrUpdateKey, new byte[] { 0 });

                        this.Log("Added {0}", addOrUpdateKey);
                    }

                    txn.CommitAsync().Wait();
                }

                using (var txn = this.CreateTransaction())
                {
                    if (!this.Contains(txn, addOrUpdateKey))
                    {
                        throw new InvalidOperationException(string.Format("AddOrUpdate({0}) key not found", addOrUpdateKey));
                    }

                    if (this.TryUpdate(txn, addOrUpdateKey, new byte[] { 0 }))
                    {
                        this.Log("Updated {0}", addOrUpdateKey);
                    }
                    else
                    {
                        throw new InvalidOperationException(string.Format("Should have updated {0}", addOrUpdateKey));
                    }

                    txn.CommitAsync().Wait();
                }

                using (var txn = this.CreateTransaction())
                {
                    if (this.TryRemove(txn, addOrUpdateKey))
                    {
                        this.Log("Removed {0}", addOrUpdateKey);
                    }
                    else
                    {
                        throw new InvalidOperationException(string.Format("Should have removed {0}", addOrUpdateKey));
                    }

                    txn.CommitAsync().Wait();
                }

                using (var txn = this.CreateTransaction())
                {
                    if (this.TryAdd(txn, addOrUpdateKey, new byte[] { 0 }))
                    {
                        this.Log("Added {0}", addOrUpdateKey);
                    }
                    else
                    {
                        throw new InvalidOperationException(string.Format("Should have added {0}", addOrUpdateKey));
                    }

                    txn.CommitAsync().Wait();
                }

                using (var txn = this.CreateTransaction())
                {
                    if (!this.TryAdd(txn, addOrUpdateKey, new byte[] { 0 }))
                    {
                        this.Update(txn, addOrUpdateKey, new byte[] { 0 });

                        this.Log("Updated on !TryAdd {0}", addOrUpdateKey);
                    }
                    else
                    {
                        throw new InvalidOperationException(string.Format("Should have updated on !TryAdd {0}", addOrUpdateKey));
                    }

                    txn.CommitAsync().Wait();
                }

                SendTestResultResponse(listenerContext, true);
            }
            catch (Exception ex)
            {
                string error = string.Format("Replication failed with {0}", ex);
                this.Log(error);

                byte[] resp = Encoding.UTF8.GetBytes(error);
                listenerContext.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
                listenerContext.Response.Close();
            }
        }

        private void TestEnsurePrimary(HttpListenerContext listenerContext)
        {
            for (var retry=0; retry<30; ++retry)
            {
                try
                {
                    using (var txn = this.CreateTransaction())
                    {
                        SendTestResultResponse(listenerContext, true);
                        return;
                    }
                }
                catch (FabricNotPrimaryException)
                {
                    Thread.Sleep(TimeSpan.FromSeconds(1));
                }
            }

            this.Log("TestEnsurePrimary: exhausted retries");

            SendTestResultResponse(listenerContext, false);
        }

        private void TestLastModifiedOnPrimary(HttpListenerContext listenerContext)
        {
            bool result = true;

            string keyPrefix = Guid.NewGuid().ToString();
            const int noOfKeys = 10;

            var lastModfiedOnPrimaryMap = new Dictionary<string, DateTime>();

            try
            {
                using (var txn = this.CreateTransaction())
                {
                    for (int i = 1; i <= noOfKeys; i++)
                    {
                        this.Add(txn, keyPrefix + i, new byte[] { 0 });
                    }

                    txn.CommitAsync().Wait();
                }

                using (var txn = this.CreateTransaction())
                {
                    var en = this.EnumerateMetadata(txn, keyPrefix);

                    while (en.MoveNext())
                    {
                        if (en.Current.LastModifiedOnPrimaryUtc == DateTime.MinValue)
                        {
                            throw new InvalidDataException(
                                string.Format("Last modified on primary value is zero after ADD. key: {0}.", en.Current.Key));
                        }

                        lastModfiedOnPrimaryMap.Add(en.Current.Key, en.Current.LastModifiedOnPrimaryUtc);
                    }

                    txn.CommitAsync().Wait();
                }

                Thread.Sleep(2000);

                using (var txn = this.CreateTransaction())
                {
                    for (int i = 1; i <= noOfKeys; i++)
                    {
                        this.Update(txn, keyPrefix + i, new byte[] { 0 });
                    }

                    txn.CommitAsync().Wait();
                }

                using (var txn = this.CreateTransaction())
                {
                    var en = this.Enumerate(txn, keyPrefix);

                    while (en.MoveNext())
                    {
                        if (en.Current.Metadata.LastModifiedOnPrimaryUtc == DateTime.MinValue)
                        {
                            throw new InvalidDataException(
                                string.Format("Last modified on primary value is zero after UPDATE. key: {0}.", en.Current.Metadata.Key));
                        }

                        if (en.Current.Metadata.LastModifiedOnPrimaryUtc <= lastModfiedOnPrimaryMap[en.Current.Metadata.Key])
                        {
                            throw new InvalidDataException(
                                string.Format("Last modified on primary value did not increase after UPDATE. key: {0}.", en.Current.Metadata.Key));
                        }
                    }

                    txn.CommitAsync().Wait();
                }

                this.Log("TestLastModifiedOnPrimary: Removing added Keys");
                using (var txn = this.CreateTransaction())
                {
                    foreach (var key in lastModfiedOnPrimaryMap.Keys)
                    {
                        this.Remove(txn, key);
                    }

                    txn.CommitAsync().Wait();
                }

                this.Log("TestLastModifiedOnPrimary: Removing Keys SUCCEEDED.");
            }
            catch (Exception ex)
            {
                string error = string.Format("TestLastModifiedOnPrimary failed with {0}", ex);
                this.Log(error);
                result = false;
            }

            SendTestResultResponse(listenerContext, result);
        }

        private void TestCommitNotCancellable(HttpListenerContext listenerContext)
        {
            bool result = true;

            string keyPrefix = Guid.NewGuid().ToString();
            const int noOfKeys = 70;
            var bytes = new byte[1024 * 1024];

            CancellationTokenSource cts = null;
            Stopwatch sw = new Stopwatch();

            try
            {
                this.Log("TestCommitNotCancellable: Testing txn.CommitAsync() is non-cancellable...");

                for (int iter = 1; iter <= 3; iter++)
                {
                    using (var txn = this.CreateTransaction())
                    {
                        for (int i = 1; i <= noOfKeys; i++)
                        {
                            var key = keyPrefix + i;
                            if (this.Contains(txn, key))
                            {
                                this.Update(txn, key, bytes);
                            }
                            else
                            {
                                this.Add(txn, keyPrefix + i, bytes);
                            }
                        }
                        
                        sw.Restart();
                        cts = new CancellationTokenSource();
                        var commitTask = txn.CommitAsync(TimeSpan.MaxValue);
                        cts.Cancel();

                        commitTask.Wait(); // Takes ~5 sec for 70MB of data.
                        sw.Stop();
                        this.Log("TestCommitNotCancellable: txn.CommitAsync() Elapsed: " + sw.ElapsedMilliseconds + "\n");
                    }
                }

                this.Log("TestCommitNotCancellable: Testing txn.CommitAsync() SUCCEEDED.");

                this.Log("TestCommitNotCancellable: Removing added Keys");

                using (var txn = this.CreateTransaction())
                {
                    for (int i = 1; i <= noOfKeys; i++)
                    {
                        this.Remove(txn, keyPrefix + i);
                    }

                    txn.CommitAsync().Wait();
                }

                this.Log("TestCommitNotCancellable: Removing Keys SUCCEEDED.");
            }
            catch (Exception ex)
            {
                string msgFormat = "TestCommitNotCancellable failed with {0}";

                var opEx = ex as OperationCanceledException;
                if (opEx != null && opEx.CancellationToken == cts.Token)
                {
                    msgFormat = "TestCommitNotCancellable: UNEXPECTED txn.CommitAsync() behavior: Got cancelled with {0}";
                }
                
                string error = string.Format(msgFormat, ex.ToString());
                this.Log(error);
                result = false;
            }
            
            SendTestResultResponse(listenerContext, result);
        }

        private void TryUpdatePerf(HttpListenerContext listenerContext)
        {
            try
            {
                string tryUpdatePerfKey = "TryUpdatePerfKey"; 

                using (var txn = this.CreateTransaction())
                {
                    this.Add(txn, tryUpdatePerfKey, new byte[] { 0 });

                    txn.CommitAsync().Wait();
                }

                Stopwatch stopwatch = new Stopwatch();
                int iterations = 100000;

                stopwatch.Restart();

                for (var ix=0; ix<iterations; ++ix)
                {
                    using (var tx = this.CreateTransaction())
                    {
                        this.Update(tx, tryUpdatePerfKey, new byte[] { 0 });
                    }
                }

                stopwatch.Stop();

                var updateElapsed = stopwatch.Elapsed;

                stopwatch.Restart();

                for (var ix=0; ix<iterations; ++ix)
                {
                    using (var tx = this.CreateTransaction())
                    {
                        this.TryUpdate(tx, tryUpdatePerfKey, new byte[] { 0 });
                    }
                }

                stopwatch.Stop();

                var tryUpdateElapsed = stopwatch.Elapsed;

                var updateThroughput = iterations / updateElapsed.TotalSeconds;
                var tryUpdateThroughput = iterations / tryUpdateElapsed.TotalSeconds;

                this.Log("Iterations={0}: Update: total={1}s throughput={2}tx/s TryUpdate: total={3} throughput={4}tx/s",
                    iterations,
                    updateElapsed,
                    updateThroughput,
                    tryUpdateElapsed,
                    tryUpdateThroughput);

                var allowedDelta = 0.10;
                var delta = Math.Abs(updateThroughput - tryUpdateThroughput) / updateThroughput;

                if (delta > allowedDelta)
                {
                    throw new InvalidOperationException(string.Format("TryUpdate/Update performance difference {0} exceeds {1}",
                        delta,
                        allowedDelta));
                }

                SendTestResultResponse(listenerContext, true);
            }
            catch (Exception ex)
            {
                string error = string.Format("Replication failed with {0}", ex);
                this.Log(error);

                byte[] resp = Encoding.UTF8.GetBytes(error);
                listenerContext.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
                listenerContext.Response.Close();
            }
        }

        private void TestContains(HttpListenerContext listenerContext)
        {
            bool result = false;
            string testKey1 = "TestKey1";
            string testKey2 = "TestKey2";

            {
                result = false;

                this.Log("Testing Contains(testKey1) expected true");

                try
                {
                    using (var tx = this.CreateTransaction())
                    {
                        this.Add(tx, testKey1, new Byte[] { 0 });

                        if (!this.TryAdd(tx, testKey2, new Byte[] { 0 }))
                        {
                            throw new InvalidOperationException(string.Format("TryAdd({0}) failed", testKey2));
                        }

                        tx.CommitAsync().Wait();
                    }

                    using (var tx = this.CreateTransaction())
                    {
                        result = this.Contains(tx, testKey1);
                        this.Log("Contains({0}) = {1}", testKey1, result);
                        this.Remove(tx, testKey1);

                        result = this.Contains(tx, testKey2);
                        this.Log("Contains({0}) = {1}", testKey2, result);
                        if (!this.TryRemove(tx, testKey2))
                        {
                            throw new InvalidOperationException(string.Format("TryRemove({0}) failed", testKey2));
                        }

                        tx.CommitAsync().Wait();
                    }
                }
                catch (Exception e)
                {
                    this.Log("Unexpected exception {0} while TestContains(true)", e);
                    result = false;
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                result = false;

                this.Log("Testing Contains(testKey1) expected false");

                try
                {
                    using (var tx = this.CreateTransaction())
                    {
                        var contains1 = this.Contains(tx, testKey1);
                        this.Log("Contains({0}) = {1}", testKey1, contains1);

                        var contains2 = this.Contains(tx, testKey2);
                        this.Log("Contains({0}) = {1}", testKey2, contains2);

                        result = (!contains1 && !contains2);
                    }
                }
                catch (Exception e)
                {
                    this.Log("Unexpected exception {0} while TestContains(false)", e);
                    result = false;
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            SendTestResultResponse(listenerContext, true);
            return;
        }

        private void TestUpdateReplicatorSettings1(HttpListenerContext listenerContext)
        {
            var result = TestExpectedException<ArgumentException>(
                "TestUpdateReplicatorSettings",
                () =>
                {
                    var settings = CreateReplicatorSettings();
                    settings.BatchAcknowledgementInterval = TimeSpan.FromMilliseconds(100);
                    this.UpdateReplicatorSettings(settings);
                });

            SendTestResultResponse(listenerContext, result);
        }

        private void TestUpdateReplicatorSettings2(HttpListenerContext listenerContext)
        {
            // This variation verifies that the call to "UpdateReplicatorSettings" succeeds, in spite of setting all the parameters to their default values
            // This is because the replicator checks the current values and if the new value is the same, ignores the update.

            // The only setting that is allowed to be changed dynamically is the security setting
            var settings = CreateReplicatorSettings();

            settings.BatchAcknowledgementInterval = TimeSpan.FromMilliseconds(15);
            settings.InitialReplicationQueueSize = 64;
            settings.InitialCopyQueueSize = 64;
            settings.MaxReplicationQueueSize = 1024;
            settings.MaxCopyQueueSize = 1024;
            settings.MaxReplicationQueueMemorySize = 0;
            // Do not set replicator address as we dont know the original value
            // settings.ReplicatorAddress = "localhost:0"; 
            settings.RequireServiceAck = false;
            settings.SecondaryClearAcknowledgedOperations = false;
            settings.UseStreamFaultsAndEndOfStreamOperationAck = false;
            settings.InitialSecondaryReplicationQueueSize = 64;
            settings.MaxSecondaryReplicationQueueSize = 2048;
            settings.MaxSecondaryReplicationQueueMemorySize = 0;
            settings.InitialPrimaryReplicationQueueSize = 64;
            settings.MaxPrimaryReplicationQueueSize = 1024;
            settings.MaxPrimaryReplicationQueueMemorySize = 0;
            settings.PrimaryWaitForPendingQuorumsTimeout = TimeSpan.Zero;

            // If the below call succeeds, no exception is thrown
            this.UpdateReplicatorSettings(settings);
            SendTestResultResponse(listenerContext, true);
        }

        private void TestOutOfBoundReplicatorSettings(HttpListenerContext listenerContext)
        {
            var result = TestExpectedException<ArgumentOutOfRangeException>(
                "TestOutofBoundReplicatorSettings",
                () =>
                {
                    var settings = new ReplicatorSettings();
                    string updatingSetting;
                
                    //Randomly update 1 of the settings to an out of bounds value
                    switch(new Random(DateTime.Now.Second).Next() % 15)
                    {
                        case 0:
                            settings.BatchAcknowledgementInterval = TimeSpan.FromMilliseconds((double)UInt32.MaxValue + 1);
                            updatingSetting = "BatchAcknowledgementInterval";
                            break;
                        case 1:
                            settings.RetryInterval = TimeSpan.FromMilliseconds((double)UInt32.MaxValue + 1);
                            updatingSetting = "RetryInterval";
                            break;
                        case 2:
                            settings.InitialCopyQueueSize = UInt32.MaxValue;
                            settings.InitialCopyQueueSize++;
                            updatingSetting = "InitialCopyQueueSize";
                            break;
                        case 3:
                            settings.InitialReplicationQueueSize = UInt32.MaxValue;
                            settings.InitialReplicationQueueSize++;
                            updatingSetting = "InitialReplicationQueueSize";
                            break;
                        case 4:
                            settings.MaxCopyQueueSize = UInt32.MaxValue;
                            settings.MaxCopyQueueSize++;
                            updatingSetting = "MaxCopyQueueSize";
                            break;
                        case 5:
                            settings.MaxReplicationMessageSize = UInt32.MaxValue;
                            settings.MaxReplicationMessageSize++;
                            updatingSetting = "MaxReplicationMessageSize";
                            break;
                        case 6:
                            settings.MaxReplicationQueueMemorySize = UInt32.MaxValue;
                            settings.MaxReplicationQueueMemorySize++;
                            updatingSetting = "MaxReplicationQueueMemorySize";
                            break;
                        case 7:
                            settings.MaxReplicationQueueSize = UInt32.MaxValue;
                            settings.MaxReplicationQueueSize++;
                            updatingSetting = "MaxReplicationQueueSize";
                            break;
                        case 8:
                            settings.InitialSecondaryReplicationQueueSize = UInt32.MaxValue;
                            settings.InitialSecondaryReplicationQueueSize++;
                            updatingSetting = "InitialSecondaryReplicationQueueSize";
                            break;
                        case 9:
                            settings.MaxSecondaryReplicationQueueSize = UInt32.MaxValue;
                            settings.MaxSecondaryReplicationQueueSize++;
                            updatingSetting = "MaxSecondaryReplicationQueueSize";
                            break;
                        case 10:
                            settings.MaxSecondaryReplicationQueueMemorySize = UInt32.MaxValue;
                            settings.MaxSecondaryReplicationQueueMemorySize++;
                            updatingSetting = "MaxSecondaryReplicationQueueMemorySize";
                            break;
                        case 11:
                            settings.InitialPrimaryReplicationQueueSize = UInt32.MaxValue;
                            settings.InitialPrimaryReplicationQueueSize++;
                            updatingSetting = "InitialPrimaryReplicationQueueSize";
                            break;
                        case 12:
                            settings.MaxPrimaryReplicationQueueSize = UInt32.MaxValue;
                            settings.MaxPrimaryReplicationQueueSize++;
                            updatingSetting = "MaxPrimaryReplicationQueueSize";
                            break;
                        case 13:
                            settings.MaxPrimaryReplicationQueueMemorySize = UInt32.MaxValue;
                            settings.MaxPrimaryReplicationQueueMemorySize++;
                            updatingSetting = "MaxPrimaryReplicationQueueMemorySize";
                            break;
                        case 14:
                            settings.PrimaryWaitForPendingQuorumsTimeout = TimeSpan.FromMilliseconds((double)UInt32.MaxValue + 1);
                            updatingSetting = "PrimaryWaitForPendingQuorumsTimeout";
                            break;
                        default:
                            throw new ArgumentOutOfRangeException("TestOutOfBoundReplicatorSettings unexpected switch case value");
                    }

                    this.Log("TestOutOfBoundReplicatorSettings: Setting replicator param {0}", updatingSetting);

                    this.UpdateReplicatorSettings(settings);
                });

            SendTestResultResponse(listenerContext, result);
        }

        private void TestKeyNotFound(HttpListenerContext listenerContext)
        {
            bool result = false;

            {
                result = TestExpectedException<FabricException>(
                    "TestKeyNotFound-Get",
                    () =>
                    {

                        using (var tx = this.CreateTransaction())
                        {
                            this.Get(tx, Guid.NewGuid().ToString());
                            tx.CommitAsync().Wait();
                        }
                    },
                    FabricErrorCode.KeyNotFound);

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                result = TestExpectedException<FabricException>(
                    "TestKeyNotFound-GetMetadata",
                    () =>
                    {

                        using (var tx = this.CreateTransaction())
                        {
                            this.GetMetadata(tx, Guid.NewGuid().ToString());
                            tx.CommitAsync().Wait();
                        }
                    },
                    FabricErrorCode.KeyNotFound);

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                result = TestExpectedException<FabricException>(
                    "TestKeyNotFound-GetValue",
                    () =>
                    {

                        using (var tx = this.CreateTransaction())
                        {
                            this.GetValue(tx, Guid.NewGuid().ToString());
                            tx.CommitAsync().Wait();
                        }
                    },
                    FabricErrorCode.KeyNotFound);

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                result = TestExpectedException<FabricException>(
                    "TestKeyNotFound-Update",
                    () =>
                    {

                        using (var tx = this.CreateTransaction())
                        {
                            this.Update(tx, Guid.NewGuid().ToString(), new byte[] { 0 });
                            tx.CommitAsync().Wait();
                        }
                    },
                    FabricErrorCode.KeyNotFound);

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                result = TestExpectedException<FabricException>(
                    "TestKeyNotFound-Remove",
                    () =>
                    {

                        using (var tx = this.CreateTransaction())
                        {
                            this.Remove(tx, Guid.NewGuid().ToString());
                            tx.CommitAsync().Wait();
                        }
                    },
                    FabricErrorCode.KeyNotFound);

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                this.Log("TestKeyNotFound-TryGet");

                using (var tx = this.CreateTransaction())
                {
                    result = (this.TryGet(tx, Guid.NewGuid().ToString()) == null);
                    tx.CommitAsync().Wait();
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                this.Log("TestKeyNotFound-TryGetMetadata");

                using (var tx = this.CreateTransaction())
                {
                    result = (this.TryGetMetadata(tx, Guid.NewGuid().ToString()) == null);
                    tx.CommitAsync().Wait();
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                this.Log("TestKeyNotFound-TryGetValue");

                using (var tx = this.CreateTransaction())
                {
                    result = (this.TryGetValue(tx, Guid.NewGuid().ToString()) == null);
                    tx.CommitAsync().Wait();
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                this.Log("TestKeyNotFound-TryUpdate");

                using (var tx = this.CreateTransaction())
                {
                    result = !this.TryUpdate(tx, Guid.NewGuid().ToString(), new byte[] { 0 });
                    tx.CommitAsync().Wait();
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            {
                this.Log("TestKeyNotFound-TryRemove");

                using (var tx = this.CreateTransaction())
                {
                    result = !this.TryRemove(tx, Guid.NewGuid().ToString());
                    tx.CommitAsync().Wait();
                }

                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            // all variations passed
            SendTestResultResponse(listenerContext, true);
        }

        private void TestZeroLengthValue(HttpListenerContext listenerContext)
        {
            bool result = false;

            {
                result = TestExpectedException<ArgumentException>(
                    "TestZeroLengthValue-Add",
                    () =>
                    {
                        using (var tx = this.CreateTransaction())
                        {
                            this.Add(tx, Guid.NewGuid().ToString(), new byte[] { });
                            tx.CommitAsync().Wait();
                        }
                    });
                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }


            {
                result = TestExpectedException<ArgumentException>(
                    "TestZeroLengthValue-Update",
                    () =>
                    {
                        using (var tx = this.CreateTransaction())
                        {
                            this.Update(tx, Guid.NewGuid().ToString(), new byte[] { });
                            tx.CommitAsync().Wait();
                        }
                    });
                if (!result)
                {
                    SendTestResultResponse(listenerContext, result);
                    return;
                }
            }

            // all variations passed
            SendTestResultResponse(listenerContext, true);
        }

        private void TestSettings(HttpListenerContext listenerContext)
        {
            this.TestEseSettings();
            this.TestKvsSettings();

            SendTestResultResponse(listenerContext, true);
        }

        private void TestEseSettings()
        {
            var expectedSettings = CreateEseSettings();
            var currentSettings = (LocalEseStoreSettings)this.LocalStoreSettings;

            this.Log("ESE settings: LogFileSizeInKB expected={0} current={1}", expectedSettings.LogFileSizeInKB, currentSettings.LogFileSizeInKB);
            this.Log("ESE settings: LogBufferSizeInKB expected={0} current={1}", expectedSettings.LogBufferSizeInKB, currentSettings.LogBufferSizeInKB);
            this.Log("ESE settings: MaxCursors expected={0} current={1}", expectedSettings.MaxCursors, currentSettings.MaxCursors);
            this.Log("ESE settings: MaxVerPages expected={0} current={1}", expectedSettings.MaxVerPages, currentSettings.MaxVerPages);
            this.Log("ESE settings: MaxAsyncCommitDelay expected={0} current={1}", expectedSettings.MaxAsyncCommitDelay, currentSettings.MaxAsyncCommitDelay);
            this.Log("ESE settings: EnableIncrementalBackup expected={0} current={1}", expectedSettings.EnableIncrementalBackup, currentSettings.EnableIncrementalBackup);
            this.Log("ESE settings: MaxCacheSizeInMB expected={0} current={1}", expectedSettings.MaxCacheSizeInMB, currentSettings.MaxCacheSizeInMB);
            this.Log("ESE settings: MaxDefragFrequencyInMinutes expected={0} current={1}", expectedSettings.MaxDefragFrequencyInMinutes, currentSettings.MaxDefragFrequencyInMinutes);
            this.Log("ESE settings: DefragThresholdInMB expected={0} current={1}", expectedSettings.DefragThresholdInMB, currentSettings.DefragThresholdInMB);
            this.Log("ESE settings: DatabasePageSizeInKB expected={0} current={1}", expectedSettings.DatabasePageSizeInKB, currentSettings.DatabasePageSizeInKB);
            this.Log("ESE settings: CompactionThresholdInMB expected={0} current={1}", expectedSettings.CompactionThresholdInMB, currentSettings.CompactionThresholdInMB);
            this.Log("ESE settings: IntrinsicValueThresholdInBytes expected={0} current={1}", expectedSettings.IntrinsicValueThresholdInBytes, currentSettings.IntrinsicValueThresholdInBytes);

            if (expectedSettings.LogFileSizeInKB != currentSettings.LogFileSizeInKB)
            {
                throw new InvalidOperationException(string.Format("LogFileSizeInKB mismatch: expected={0} current={1}",
                    expectedSettings.LogFileSizeInKB,
                    currentSettings.LogFileSizeInKB));
            }

            if (expectedSettings.LogBufferSizeInKB != currentSettings.LogBufferSizeInKB)
            {
                throw new InvalidOperationException(string.Format("LogBufferSizeInKB mismatch: expected={0} current={1}",
                    expectedSettings.LogBufferSizeInKB,
                    currentSettings.LogBufferSizeInKB));
            }

            if (expectedSettings.MaxCursors != currentSettings.MaxCursors)
            {
                throw new InvalidOperationException(string.Format("MaxCursors mismatch: expected={0} current={1}",
                    expectedSettings.MaxCursors,
                    currentSettings.MaxCursors));
            }

            if (expectedSettings.MaxVerPages != currentSettings.MaxVerPages)
            {
                throw new InvalidOperationException(string.Format("MaxVerPages mismatch: expected={0} current={1}",
                    expectedSettings.MaxVerPages,
                    currentSettings.MaxVerPages));
            }

            if (expectedSettings.MaxAsyncCommitDelay != currentSettings.MaxAsyncCommitDelay)
            {
                throw new InvalidOperationException(string.Format("MaxAsyncCommitDelay mismatch: expected={0} current={1}",
                    expectedSettings.MaxAsyncCommitDelay,
                    currentSettings.MaxAsyncCommitDelay));
            }

            if (expectedSettings.EnableIncrementalBackup != currentSettings.EnableIncrementalBackup)
            {
                throw new InvalidOperationException(string.Format("EnableIncrementalBackup mismatch: expected={0} current={1}",
                    expectedSettings.EnableIncrementalBackup,
                    currentSettings.EnableIncrementalBackup));
            }

            if (expectedSettings.MaxCacheSizeInMB != currentSettings.MaxCacheSizeInMB)
            {
                throw new InvalidOperationException(string.Format("MaxCacheSizeInMB mismatch: expected={0} current={1}",
                    expectedSettings.MaxCacheSizeInMB,
                    currentSettings.MaxCacheSizeInMB));
            }

            if (expectedSettings.MaxDefragFrequencyInMinutes != currentSettings.MaxDefragFrequencyInMinutes)
            {
                throw new InvalidOperationException(string.Format("MaxDefragFrequencyInMinutes mismatch: expected={0} current={1}",
                    expectedSettings.MaxDefragFrequencyInMinutes,
                    currentSettings.MaxDefragFrequencyInMinutes));
            }

            if (expectedSettings.DefragThresholdInMB != currentSettings.DefragThresholdInMB)
            {
                throw new InvalidOperationException(string.Format("DefragThresholdInMB mismatch: expected={0} current={1}",
                    expectedSettings.DefragThresholdInMB,
                    currentSettings.DefragThresholdInMB));
            }

            if (expectedSettings.DatabasePageSizeInKB != currentSettings.DatabasePageSizeInKB)
            {
                throw new InvalidOperationException(string.Format("DatabasePageSizeInKB mismatch: expected={0} current={1}",
                    expectedSettings.DatabasePageSizeInKB,
                    currentSettings.DatabasePageSizeInKB));
            }

            if (expectedSettings.CompactionThresholdInMB != currentSettings.CompactionThresholdInMB)
            {
                throw new InvalidOperationException(string.Format("CompactionThresholdInMB mismatch: expected={0} current={1}",
                    expectedSettings.CompactionThresholdInMB,
                    currentSettings.CompactionThresholdInMB));
            }

            if (expectedSettings.IntrinsicValueThresholdInBytes != currentSettings.IntrinsicValueThresholdInBytes)
            {
                throw new InvalidOperationException(string.Format("IntrinsicValueThresholdInBytes mismatch: expected={0} current={1}",
                    expectedSettings.IntrinsicValueThresholdInBytes,
                    currentSettings.IntrinsicValueThresholdInBytes));
            }
        }

        private void TestKvsSettings()
        {
            var expectedSettings = CreateKvsSettings();
            var currentSettings = this.KeyValueStoreReplicaSettings;

            this.Log("KVS settings: TransactionDrainTimeout expected={0} current={1}", expectedSettings.TransactionDrainTimeout, currentSettings.TransactionDrainTimeout);
            this.Log("KVS settings: SecondaryNotificationMode expected={0} current={1}", expectedSettings.SecondaryNotificationMode, currentSettings.SecondaryNotificationMode);
            this.Log("KVS settings: EnableCopyNotificationPrefetch expected={0} current={1}", expectedSettings.EnableCopyNotificationPrefetch, currentSettings.EnableCopyNotificationPrefetch);
            this.Log("KVS settings: FullCopyMode expected={0} current={1}", expectedSettings.FullCopyMode, currentSettings.FullCopyMode);

            if (expectedSettings.TransactionDrainTimeout != currentSettings.TransactionDrainTimeout)
            {
                throw new InvalidOperationException(string.Format("TransactionDrainTimeout mismatch: expected={0} current={1}",
                    expectedSettings.TransactionDrainTimeout,
                    currentSettings.TransactionDrainTimeout));
            }

            if (expectedSettings.SecondaryNotificationMode != currentSettings.SecondaryNotificationMode)
            {
                throw new InvalidOperationException(string.Format("SecondaryNotificationMode mismatch: expected={0} current={1}",
                    expectedSettings.SecondaryNotificationMode,
                    currentSettings.SecondaryNotificationMode));
            }

            if (expectedSettings.EnableCopyNotificationPrefetch != currentSettings.EnableCopyNotificationPrefetch)
            {
                throw new InvalidOperationException(string.Format("EnableCopyNotificationPrefetch mismatch: expected={0} current={1}",
                    expectedSettings.EnableCopyNotificationPrefetch,
                    currentSettings.EnableCopyNotificationPrefetch));
            }

            if (expectedSettings.FullCopyMode != currentSettings.FullCopyMode)
            {
                throw new InvalidOperationException(string.Format("FullCopyMode mismatch: expected={0} current={1}",
                    expectedSettings.FullCopyMode,
                    currentSettings.FullCopyMode));
            }
        }

        private bool TestExpectedException<TException>(
            string tag,
            Action action) where TException : class
        {
            return TestExpectedException<TException>(tag, action, FabricErrorCode.Unknown);
        }

        private bool TestExpectedException<TException>(
            string tag,
            Action action,
            FabricErrorCode expectedErrorCode) where TException : class
        {
            bool result = false;
            string message = null;
            try
            {
                action();
                message = string.Format("Expected exception {0} for {1} but NO exception was thrown",
                    typeof(TException).Name, tag);
                result = false;
            }
            catch (FabricException ex)
            {
                TException typedException = ex as TException;
                if ((typedException != null) && (ex.ErrorCode == expectedErrorCode))
                {
                    message = string.Format("Expected exception {0} received for {1}", ex.GetType().Name, tag);
                    result = true;
                }
                else
                {
                    message = string.Format("Unexcepted exception {0} or ErrorCode {2} received for {1}",
                        ex.GetType().Name, tag, ex.ErrorCode);
                    result = false;
                }
            }
            catch (AggregateException ex)
            {
                ex.Handle(x =>
                {
                    var typedException = x as TException;
                    if (typedException != null)
                    {
                        message = string.Format("Expected exception {0} wrapped in an AggregateException received for {1}", x.GetType().Name, tag);
                        result = true;
                    }
                    else
                    {
                        message = string.Format("Unexcepted exception {0} wrapped in an AggregateException received for {1}",
                            x.GetType().Name, tag);
                        result = false;
                    }

                    return result;
                });
            }
            catch (Exception ex)
            {
                TException typedException = ex as TException;
                if (typedException != null)
                {
                    message = string.Format("Expected exception {0} received for {1}", ex.GetType().Name, tag);
                    result = true;
                }
                else
                {
                    message = string.Format("Unexcepted exception {0} received for {1}", ex.GetType().Name, tag);
                    result = false;
                }
            }

            this.Log(tag + ":" + result);
            this.Log(message);

            return result;
        }

        private void UpdatePostBackupHandlerValue(HttpListenerContext listenerContext, bool newValue)
        {
            postBackupHandlerReturnValue = newValue;
            SendTestResultResponse(listenerContext, true);
        }

        private async void TestBasicBackup(HttpListenerContext listenerContext, StoreBackupOption backupOption)
        {
            try
            {
                await DoBackup(listenerContext, backupOption);
                SendTestResultResponse(listenerContext, true);
            }
            catch (Exception e)
            {
                HandleException(listenerContext, "Backup", e);
            }
        }

        private async Task DoBackup(HttpListenerContext listenerContext, StoreBackupOption backupOption, string backupDir = null, Func<StoreBackupInfo, Task<bool>> postBackupAsyncFunc = null)
        {       
            switch (ReplicaRole)
            {
                case ReplicaRole.Primary:
                    {
                        // TruncateLogsOnly requires a null backup dir
                        if (backupOption != StoreBackupOption.TruncateLogsOnly)
                        {
                            if (backupDir == null)
                            {
                                string dir = Path.GetDirectoryName(logFile);
                                string suffix = string.Format(
                                    CultureInfo.InvariantCulture,
                                    "{0}_P{1}_R{2}_{3}",
                                    this.StoreName,
                                    partitionId,
                                    replicaId,
                                    DateTimeOffset.UtcNow.ToString("yyyyMMdd-HHmmss.fffffffZ"));

                                backupDir = Path.Combine(dir, suffix);
                            }
                        }
                    }
                    break;

                case ReplicaRole.IdleSecondary:
                case ReplicaRole.ActiveSecondary:
                    backupOption = StoreBackupOption.TruncateLogsOnly;
                    break;

                default:
                    SendTestResultResponse(listenerContext, false);
                    return;
            }

            Log(
                "Invoking DoBackup in service's {0} replica. Backup dir: {1}, backup option: {2}",
                ReplicaRole,
                string.IsNullOrWhiteSpace(backupDir) ? "<null>" : backupDir,
                backupOption);

            await BackupAsync(
                backupDir, 
                backupOption, 
                postBackupAsyncFunc ?? DefaultPostBackupAsyncHandler, 
                CancellationToken.None);

            Log(
                "DoBackup completed. Backup dir: {0}, backup option: {1}",
                string.IsNullOrWhiteSpace(backupDir) ? "<null>" : backupDir,
                backupOption);
        }

        private void TestIncrementalBackupAfterTruncateLogsOnly(HttpListenerContext listenerContext)
        {
            var result = TestExpectedException<FabricMissingFullBackupException>(
                "TestIncrementalBackupAfterTruncateLogsOnly",
                () => DoBackup(listenerContext, StoreBackupOption.Incremental).Wait(),
                FabricErrorCode.MissingFullBackup);

            SendTestResultResponse(listenerContext, result);
        }
        
        private async Task<bool> WaitingPostBackupHandler(StoreBackupInfo info)
        {
            backupInProgressTestingWaiter.WaitOne();

            var task = Task.Factory.StartNew(() => true);
            return await task;            
        }

        /// <summary>
        /// Do a backup, while the backup hasn't finished, initiate another backup. 
        /// The 2nd one should fail. Then complete the first one and try a 3rd backup.
        /// The 3rd one should succeed.
        /// </summary>
        /// <param name="listenerContext">The listener context.</param>
        /// <returns>A task for the calling method to wait on.</returns>
        private async Task TestBackupInProgressException(HttpListenerContext listenerContext)
        {
            Task t1 = DoBackup(listenerContext, StoreBackupOption.Full, null, WaitingPostBackupHandler);

            var result = TestExpectedException<FabricBackupInProgressException>(
                "TestBackupInProgressException",
                () => DoBackup(listenerContext, StoreBackupOption.Full).Wait(),
                FabricErrorCode.BackupInProgress);

            backupInProgressTestingWaiter.Set();
            await t1;
            backupInProgressTestingWaiter.Reset();

            // This should succeed since no other backup is in progress
            DoBackup(listenerContext, StoreBackupOption.Full).Wait();

            SendTestResultResponse(listenerContext, result);
        }

        private void TestBackupDirectoryNotEmptyException(HttpListenerContext listenerContext)
        {
            string dir = Path.GetDirectoryName(logFile);
            string suffix = string.Format(
                CultureInfo.InvariantCulture,
                "{0}_P{1}_R{2}_{3}",
                this.StoreName,
                partitionId,
                replicaId,
                DateTimeOffset.UtcNow.ToString("yyyyMMdd-HHmmss.fffffffZ"));

            string backupDirFull = Path.Combine(dir, suffix + "_Full");
            string backupDirIncr = Path.Combine(dir, suffix + "_Incr");
            Directory.CreateDirectory(backupDirFull);
            Directory.CreateDirectory(backupDirIncr);

            // these succeed since the backup directory is empty.
            DoBackup(listenerContext, StoreBackupOption.Full, backupDirFull).Wait();
            DoBackup(listenerContext, StoreBackupOption.Incremental, backupDirIncr).Wait();

            // create a blank file
            using (new StreamWriter(Path.Combine(backupDirFull, "temp.txt"))) {  }

            // this succeeds since ESE doesn't care about a dir not being empty for a full backup, only for incrementals
            DoBackup(listenerContext, StoreBackupOption.Full, backupDirFull).Wait();

            var result = TestExpectedException<FabricBackupDirectoryNotEmptyException>(
                "TestBackupDirectoryNotEmptyException",
                () => DoBackup(listenerContext, StoreBackupOption.Incremental, backupDirFull).Wait(),
                FabricErrorCode.BackupDirectoryNotEmpty);

            SendTestResultResponse(listenerContext, result);
        }

        private void TestIncrementalBackupAfterFalseReturnFromPostBackupHandler(HttpListenerContext listenerContext)
        {
            var result = TestExpectedException<FabricMissingFullBackupException>(
                "TestIncrementalBackupAfterFalseReturnFromPostBackupHandler",
                () => DoBackup(listenerContext, StoreBackupOption.Incremental).Wait(),
                FabricErrorCode.MissingFullBackup);

            SendTestResultResponse(listenerContext, result);
        }

        private async Task<bool> DefaultPostBackupAsyncHandler(StoreBackupInfo info)
        {
            Log("Inside the postbackup async handler in the service");

            var task = Task.Factory.StartNew(() => postBackupHandlerReturnValue);
            bool result = await task;

            Log("Returning {0} from the postbackup async handler", result);

            return result;
        }

        private void HandleException(HttpListenerContext listenerContext, string content, Exception ex)
        {
            string error = string.Format("Test {0} failed with {1}", content, ex);
            Log(error);

            byte[] resp = Encoding.UTF8.GetBytes(error);
            listenerContext.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
            listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
            listenerContext.Response.Close();
        }

        private static void SendTestResultResponse(HttpListenerContext listenerContext, bool result)
        {
            byte[] responseBytes = result ? Encoding.UTF8.GetBytes("PASSED") : Encoding.UTF8.GetBytes("FAILED");
            listenerContext.Response.StatusCode = (int)HttpStatusCode.OK;
            listenerContext.Response.OutputStream.Write(responseBytes, 0, responseBytes.Length);
            listenerContext.Response.Close();
        }

        private void DoReportFault(HttpListenerContext listenerContext)
        {
            string response = "ReportFault processed";
            byte[] resp = Encoding.UTF8.GetBytes(response);
            listenerContext.Response.StatusCode = (int)HttpStatusCode.OK;
            listenerContext.Response.OutputStream.Write(resp, 0, resp.Length);
            listenerContext.Response.Close();

            partition.ReportFault(FaultType.Transient);

            // Wait for close to be received before returning
            closeEvent.WaitOne();
        }

        private void Log(string format, params object[] args)
        {
            string msg1 = string.Format(format, args);
            string msg = string.Format("[{0}]: {1}", DateTime.Now.ToString("hh:mm:ss.ffff"), msg1);
            Trace.WriteLine(msg);

            if (!string.IsNullOrEmpty(this.logFile))
            {
                lock (this.syncLock)
                {
                    File.AppendAllLines(this.logFile, new string[] { msg });
                }
            }
        }
    }    
}