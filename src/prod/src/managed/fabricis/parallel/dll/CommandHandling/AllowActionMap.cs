// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// Contains entries of the user specified job Id/UD and action that must be allowed even though
    /// the reconcilation and policies (such as JobBlockingPolicy) had a different opinion.
    /// </summary>
    internal sealed class AllowActionMap : IAllowActionMap
    {
        private readonly IDictionary<Guid, IAllowActionRecord> records =
            new Dictionary<Guid, IAllowActionRecord>();

        private readonly object locker = new object();

        public void Set(IAllowActionRecord record)
        {
            lock (locker)
            {
                records[record.JobId] = record;
            }
        }

        public void Remove(Guid jobId)
        {
            lock (locker)
            {
                records.Remove(jobId);
            }
        }

        public IAllowActionRecord Get(Guid jobId, uint? ud)
        {
            lock (locker)
            {
                IAllowActionRecord result = null;
                if (records.TryGetValue(jobId, out result) &&
                    (result.UD == null || ud == result.UD))
                {
                    return result;
                }
                return null;
            }
        }

        public IReadOnlyList<IAllowActionRecord> Get()
        {
            lock (locker)
            {
                return records.Values.ToList();
            }
        }

        public void Clear()
        {
            lock (locker)
            {
                records.Clear();
            }
        }
    }
}