// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.IO;
    using System.Linq;
    using System.Threading;

    /// <summary>
    /// Class containing methods to help manage disk space used by DCA.
    ///
    /// Entries are removed based on chronological ordering of the files 
    /// last write time. For example, if a trace file covers 11:00am
    /// to 3:00pm, 3:00pm would be considered its timestamp. 
    /// 
    /// </summary>
    internal class DiskSpaceManager : IDisposable
    {
        private const long GB = 1 << 30;
        private const long MB = 1 << 20;

        private const double DefaultDiskQuotaUsageTargetPercent = 80;
        private const long DefaultMaxDiskQuota = 64 * GB;
        private const long DefaultMinimumRemainingDiskSpace = 1 * GB;

        private const string MaxDiskQuotaParamName = "MaxDiskQuotaInMB";
        private const string DiskFullSafetySpaceParamName = "DiskFullSafetySpaceInMB";

        private const string HealthSubProperty = "DiskSpaceAvailable";
        private const string TraceType = "DiskSpaceManager";

        private static readonly TimeSpan DefaultCleanupInterval = TimeSpan.FromMinutes(5);
        private static readonly Func<FileInfo, bool> DefaultSafeToDeleteCallback = f => true;
        private static readonly Func<FileInfo, bool> DefaultFileRetentionPolicy = f => true;
        private static readonly Func<FileInfo, bool> DefaultDeleteFunc = f =>
            {
                try
                {
                    FabricFile.Delete(f.FullName);
                    return true;
                }
                catch (Exception)
                {
                    return false;
                }
            };

        private readonly CachedValue<long> cachedCurrentMaxDiskQuota;
        private readonly CachedValue<long> cachedCurrentDiskFullSafetySpace;
        private readonly CachedValue<double> cachedDiskQuotaUsageTargetPercent;

        private readonly ConcurrentDictionary<string, ConcurrentBag<RegistrationMetadata>> managedUserFolders = 
            new ConcurrentDictionary<string, ConcurrentBag<RegistrationMetadata>>();

        private readonly DcaTimer cleanupTimer;
        private readonly FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

        // Used to reduce noisy clear health report messages.
        private bool healthOkSent;

        private bool disposed;

        public DiskSpaceManager()
            : this(
            () => MB * Utility.GetUnencryptedConfigValue(
                                    ConfigReader.DiagnosticsSectionName,
                                    MaxDiskQuotaParamName,
                                    DefaultMaxDiskQuota / MB),
            () => MB * Utility.GetUnencryptedConfigValue(
                ConfigReader.DiagnosticsSectionName,
                DiskFullSafetySpaceParamName,
                DefaultMinimumRemainingDiskSpace / MB),
            () => DefaultDiskQuotaUsageTargetPercent,
            DefaultCleanupInterval)
        {
        }

        public DiskSpaceManager(
            Func<long> getCurrentMaxDiskQuota,
            Func<long> getCurrentDiskFullSafetySpace,
            Func<double> getDiskQuotaUsageTargetPercent,
            TimeSpan cleanupInterval)
        {
            this.cachedCurrentMaxDiskQuota = new CachedValue<long>(getCurrentMaxDiskQuota);
            this.cachedCurrentDiskFullSafetySpace = new CachedValue<long>(getCurrentDiskFullSafetySpace);
            this.cachedDiskQuotaUsageTargetPercent = new CachedValue<double>(getDiskQuotaUsageTargetPercent);

            this.GetAvailableSpace = d => new DriveInfo(d.FullName).AvailableFreeSpace;

            this.cleanupTimer = new DcaTimer("DiskSpaceManager.CleanupTimer", this.OnTimer, cleanupInterval);
            this.cleanupTimer.Start();
        }

        public event Action RetentionPassCompleted;

        public long MaxDiskQuota
        {
            get { return this.cachedCurrentMaxDiskQuota.LastValue; }
        }

        public long DiskFullSafetySpace
        {
            get { return this.cachedCurrentDiskFullSafetySpace.LastValue; }
        }

        public double DiskQuotaUsageTargetPercent
        {
            get { return this.cachedDiskQuotaUsageTargetPercent.LastValue; }
        }

        internal Func<DirectoryInfo, long> GetAvailableSpace { get; set; }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (this.cleanupTimer == null)
            {
                return;
            }

            this.cleanupTimer.StopAndDispose();
            this.cleanupTimer.DisposedEvent.WaitOne();
        }

        public void RegisterFolder(
            string user,
            DirectoryInfo directory,
            string filter = null,
            Func<FileInfo, bool> safeToDeleteCallback = null,
            Func<FileInfo, bool> fileRetentionPolicy = null,
            Func<FileInfo, bool> deleteFunc = null)
        {
            if (directory == null)
            {
                throw new ArgumentNullException("directory");
            }

            if (string.IsNullOrEmpty(filter))
            {
                filter = "*";
            }

            this.RegisterFolder(
                user,
                () => directory.EnumerateFiles(filter),
                safeToDeleteCallback,
                fileRetentionPolicy,
                deleteFunc);
        }

        public void RegisterFolder(
            string user,
            Func<IEnumerable<FileInfo>> getFilesCallback,
            Func<FileInfo, bool> safeToDeleteCallback = null,
            Func<FileInfo, bool> fileRetentionPolicy = null,
            Func<FileInfo, bool> deleteFunc = null)
        {
            if (string.IsNullOrEmpty(user))
            {
                throw new ArgumentNullException("user");
            }

            if (getFilesCallback == null)
            {
                throw new ArgumentNullException("getFilesCallback");
            }

            if (safeToDeleteCallback == null)
            {
                safeToDeleteCallback = DefaultSafeToDeleteCallback;
            }

            if (fileRetentionPolicy == null)
            {
                fileRetentionPolicy = DefaultFileRetentionPolicy;
            }

            if (deleteFunc == null)
            {
                deleteFunc = DefaultDeleteFunc;
            }

            var metadata = new RegistrationMetadata(
                getFilesCallback,
                safeToDeleteCallback,
                fileRetentionPolicy,
                deleteFunc);
            var newValue = new ConcurrentBag<RegistrationMetadata> { metadata };
            this.managedUserFolders.AddOrUpdate(
                user, 
                newValue, 
                (k, v) =>
                    {
                        v.Add(metadata);
                        return v;
                    });
        }

        public void UnregisterFolders(
            string user)
        {
            ConcurrentBag<RegistrationMetadata> unused;
            this.managedUserFolders.TryRemove(user, out unused);
        }

        private void OnTimer(object state)
        {
            // Apply individual file retention policies.
            var stats = this.WalkFiles(
                (f, md, u, s) =>
                {
                    // Safe to delete and not needed to be retained.
                    if (md.SafeToDeleteCallback(f) && !md.FileRetentionPolicy(f))
                    {
                        if (md.DeleteFunc(f))
                        {
                            return new { Attempted = s.Attempted + 1, Successful = s.Successful + 1 };
                        }

                        return new { Attempted = s.Attempted + 1, s.Successful };
                    }

                    return new { s.Attempted, s.Successful };
                }, 
                new { Attempted = 0, Successful = 0 });

            this.traceSource.WriteInfo(
                TraceType,
                "Local policy deleted {0} of {1} attempted files from the directory.",
                stats.Successful,
                stats.Attempted);

            // Apply global file retention policy.
            var cleanupRequired = this.CalculateCleanupRequired();

            var bytesDeleted = this.DeleteForGlobalPolicy(cleanupRequired, true);

            // If we were unable to delete enough files safely, ignore safety.
            var remaining = cleanupRequired - bytesDeleted;
            if (remaining > 0)
            {
                this.DeleteForGlobalPolicy(remaining, false);
            }

            var completed = this.RetentionPassCompleted;
            if (completed != null)
            {
                completed();
            }

            this.cleanupTimer.Start();
        }

        private long DeleteForGlobalPolicy(long cleanupRequired, bool safeToDeleteFilesOnly)
        {
            var tokenSource = new CancellationTokenSource();
            var results = this.WalkChronologicalFileList(
                (f, md, u, deleteStats) =>
                {
                    var remaining = deleteStats.Remaining;
                    if (remaining <= 0)
                    {
                        tokenSource.Cancel();
                        return deleteStats;
                    }

                    var attempts = deleteStats.Attempted + 1;
                    var successful = deleteStats.Successful;
                    var size = f.Length;
                    var threshold = deleteStats.NewThreshold;
                    if (md.DeleteFunc(f))
                    {
                        remaining -= size;
                        threshold = f.LastWriteTimeUtc;
                        successful++;

                        if (!safeToDeleteFilesOnly)
                        {
                            this.traceSource.WriteInfo(
                                TraceType,
                                "Deleted a file not marked as safe to delete to satisfy global policy. File: {0}, User: {1}, LastWriteTimeUtc: {2}",
                                f.Name,
                                u,
                                f.LastWriteTimeUtc);
                        }
                    }

                    return new { Remaining = remaining, Attempted = attempts, Successful = successful, NewThreshold = threshold };
                },
                safeToDeleteFilesOnly,
                new { Remaining = cleanupRequired, Attempted = 0, Successful = 0, NewThreshold = DateTime.MinValue },
                tokenSource.Token);

            FabricEvents.Events.DiskSpaceManagerGlobalPolicyStats(
                safeToDeleteFilesOnly,
                results.Successful,
                results.Attempted,
                cleanupRequired - results.Remaining,
                cleanupRequired,
                (DateTime.UtcNow - results.NewThreshold).TotalMinutes);

            // return bytes deleted
            return cleanupRequired - results.Remaining;
        }

        private long CalculateCleanupRequired()
        {
            var directoryInfo = new DirectoryInfo(Utility.LogDirectory);
            var diskSpaceRemaining = this.GetAvailableSpace(directoryInfo);

            // The DCA "diskfullsafetyspace" will help with situations where other files 
            // are filling the drive or DCA's quota is >disk size.  The quota mechanism would 
            // prevent disk full otherwise.  Running the cleanup more often makes sure that we 
            // stay closer to our target usage (less deviation).
            // This check isFilter specified, but doesn't match any registered for safety so we ignore fairness.
            var fullProtectionDiskSpaceToFree = Math.Max(
                this.cachedCurrentDiskFullSafetySpace.Update() - diskSpaceRemaining,
                0);

            var diskQuota = this.cachedCurrentMaxDiskQuota.Update();

            // We have exceeded the disk quota and need to free up some disk space
            var targetDiskUsage = (long)(diskQuota * this.cachedDiskQuotaUsageTargetPercent.Update() / 100);

            // Figure out how much disk space we need to free up for quota
            var usageDictionary = new Dictionary<string, long>();
            this.WalkFiles(
                (f, md, u, dict) =>
                {
                    if (!dict.ContainsKey(u))
                    {
                        dict[u] = 0;
                    }

                    dict[u] += f.Length;
                    return dict;
                }, 
                usageDictionary);

            foreach (var userCount in usageDictionary)
            {
                this.traceSource.WriteInfo(
                    TraceType,
                    "Disk space user {0} was found to be using {1:N0}B of disk space.", 
                    userCount.Key,
                    userCount.Value);
            }

            var totalDiskSpaceUsed = usageDictionary.Values.Sum();

            if (fullProtectionDiskSpaceToFree > 0)
            {
                HealthClient.SendNodeHealthReport(
                    StringResources.DCAWarning_InsufficientDiskSpaceHealthDescription,
                    HealthState.Warning,
                    HealthSubProperty);
                this.healthOkSent = false;
                this.traceSource.WriteWarning(
                    TraceType, 
                    "Insufficient space found for logs. Disk space remaining: {0}. Disk space to free: {1}.", 
                    diskSpaceRemaining,
                    fullProtectionDiskSpaceToFree);
            }
            else
            {
                if (!this.healthOkSent)
                {
                    HealthClient.ClearNodeHealthReport(HealthSubProperty);
                    this.healthOkSent = true;
                    this.traceSource.WriteInfo(
                        TraceType,
                        "Space found for logs. Disk space remaining: {0:N0}B.",
                        diskSpaceRemaining);
                }
            }

            return Math.Max(totalDiskSpaceUsed - targetDiskUsage, fullProtectionDiskSpaceToFree);
        }

        private T WalkChronologicalFileList<T>(
            Func<FileInfo, RegistrationMetadata, string, T, T> observer, 
            bool safeToDeleteFilesOnly,
            T initialState = default(T),
            CancellationToken cancellationToken = default(CancellationToken))
        {
            if (cancellationToken == default(CancellationToken))
            {
                cancellationToken = CancellationToken.None;
            }

            var list = new List<Tuple<FileInfo, RegistrationMetadata, string>>();
            this.WalkFiles(
                (f, md, u, l) =>
                {
                    // If we care about safety, check with callback
                    if (!safeToDeleteFilesOnly || md.SafeToDeleteCallback(f))
                    {
                        l.Add(new Tuple<FileInfo, RegistrationMetadata, string>(f, md, u));
                    }

                    return l;
                }, 
                list);

            T state = initialState;
            foreach (var tuple in list.OrderBy(pair => pair.Item1.LastWriteTimeUtc))
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                state = observer(tuple.Item1, tuple.Item2, tuple.Item3, state);
            }

            return state;
        }

        private T WalkFiles<T>(Func<FileInfo, RegistrationMetadata, string, T, T> observer, T initialState = default(T))
        {
            T state = initialState;
            foreach (var user in this.managedUserFolders)
            {
                foreach (var metadata in user.Value)
                {
                    foreach (var file in metadata.GetFilesCallback())
                    {
                        state = observer(file, metadata, user.Key, state);
                    }
                }
            }

            return state;
        }

        private class RegistrationMetadata
        {
            private readonly Func<IEnumerable<FileInfo>> getFilesCallback;
            private readonly Func<FileInfo, bool> safeToDeleteCallback;
            private readonly Func<FileInfo, bool> fileRetentionPolicy;
            private readonly Func<FileInfo, bool> deleteFunc;

            public RegistrationMetadata(
                Func<IEnumerable<FileInfo>> getFilesCallback,
                Func<FileInfo, bool> safeToDeleteCallback,
                Func<FileInfo, bool> fileRetentionPolicy,
                Func<FileInfo, bool> deleteFunc)
            {
                this.getFilesCallback = getFilesCallback;
                this.safeToDeleteCallback = safeToDeleteCallback;
                this.fileRetentionPolicy = fileRetentionPolicy;
                this.deleteFunc = deleteFunc;
            }

            public Func<IEnumerable<FileInfo>> GetFilesCallback
            {
                get { return this.getFilesCallback; }
            }

            public Func<FileInfo, bool> SafeToDeleteCallback
            {
                get { return this.safeToDeleteCallback; }
            }

            public Func<FileInfo, bool> FileRetentionPolicy
            {
                get { return this.fileRetentionPolicy; }
            }

            public Func<FileInfo, bool> DeleteFunc
            {
                get { return this.deleteFunc; }
            }
        }

        private class CachedValue<T>
        {
            private readonly Func<T> updateValue;
            private T cachedValue;

            public CachedValue(Func<T> updateValue)
            {
                this.updateValue = updateValue;
                this.cachedValue = updateValue();
            }

            public T LastValue
            {
                get { return this.cachedValue; }
            }

            public T Update()
            {
                this.cachedValue = this.updateValue();
                return this.cachedValue;
            }
        }
    }
}