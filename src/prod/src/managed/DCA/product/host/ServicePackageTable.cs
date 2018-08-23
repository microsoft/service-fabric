// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Linq;

    // Class that implements the service package table 
    internal class ServicePackageTable
    {
        private readonly Dictionary<string, ServicePackageTableRecord> table;

        internal ServicePackageTable()
        {
            this.table = new Dictionary<string, ServicePackageTableRecord>();
        }

        internal void AddOrUpdateRecord(string tableKey, ServicePackageTableRecord record)
        {
            lock (this.table)
            {
                record.LatestNotificationTimestamp = DateTime.UtcNow;
                this.table[tableKey] = record;
            }
        }

        internal void RemoveRecord(string tableKey)
        {
            lock (this.table)
            {
                this.table.Remove(tableKey);
            }
        }

        internal Dictionary<string, ServicePackageTableRecord> GetAllRecords()
        {
            Dictionary<string, ServicePackageTableRecord> records;
            lock (this.table)
            {
                records = new Dictionary<string, ServicePackageTableRecord>(this.table);
            }

            return records;
        }

        internal List<ServicePackageTableRecord> GetRecordsForAppInstance(string nodeName, string appInstanceId)
        {
            List<ServicePackageTableRecord> records;
            lock (this.table)
            {
                records = this.table.Values
                          .Where(s => (s.NodeName.Equals(nodeName, StringComparison.Ordinal) &&
                                       s.ApplicationInstanceId.Equals(appInstanceId, StringComparison.Ordinal)))
                          .ToList();
            }

            return records;
        }

        internal void MarkInactiveRecordsForDeletion(DateTime cutoffTime)
        {
            lock (this.table)
            {
                foreach (ServicePackageTableRecord record in this.table.Values)
                {
                    if (record.LatestNotificationTimestamp.CompareTo(cutoffTime) < 0)
                    {
                        FabricEvents.Events.ServicePackageInactive(
                            0,
                            record.NodeName,
                            record.ApplicationInstanceId,
                            record.ServicePackageName);
                    }
                }
            }
        }
    }
}