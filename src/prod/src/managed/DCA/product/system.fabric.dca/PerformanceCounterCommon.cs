// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    internal class PerformanceCounterCommon
    {
        internal const int MaxPerformanceCountersBinaryFileSizeInMB = 50;

        internal const string LabelDefault = "_default_";
        internal const string LabelHighStoreDensity = "_highstoredensity_";
        internal const string LabelHighDensity = "_highdensity_";
        internal const string PerformanceCounterSectionName = "PerformanceCounterLocalStore";
        internal static readonly TimeSpan DefaultSamplingInterval = TimeSpan.FromSeconds(60);
        internal static readonly TimeSpan NewPerfCounterBinaryFileCreationInterval = TimeSpan.FromMinutes(45);

        private const string GroupCore = "#core";
        private const string GroupPerPartition = "#partition";
        private const string GroupPerReplica = "#replica";
        private const string GroupPerStore = "#store";

        private static readonly IReadOnlyDictionary<string, string[]> CounterLabels = new ReadOnlyDictionary<string, string[]>(new Dictionary<string, string[]>()
        {
            { LabelDefault,             new[] { GroupCore, GroupPerPartition, GroupPerReplica, GroupPerStore } },
            { LabelHighStoreDensity,    new[] { GroupCore, GroupPerPartition, GroupPerReplica } },
            { LabelHighDensity,         new[] { GroupCore } }
        });

        private static readonly IReadOnlyDictionary<string, string[]> CounterGroups = new ReadOnlyDictionary<string, string[]>(new Dictionary<string, string[]>()
        {
            { 
                GroupCore, 
                new[] 
                {
                    @"\Processor(_Total)\% Processor Time",
                    @"\TCPv4\Connections Active",
                    @"\TCPv6\Connections Active",
                    @"\TCPv4\Connections Passive",
                    @"\TCPv6\Connections Passive",
                    @"\TCPv4\Segments Sent/sec",
                    @"\TCPv6\Segments Sent/sec",
                    @"\TCPv4\Segments Received/sec",
                    @"\TCPv6\Segments Received/sec",
                    @"\TCPv4\Segments Retransmitted/sec",
                    @"\TCPv6\Segments Retransmitted/sec",
                    @"\TCPv4\Connection Failures",
                    @"\TCPv6\Connection Failures",
                    @"\TCPv4\Connections Reset",
                    @"\TCPv6\Connections Reset",
                    @"\Event Tracing for Windows Session(FabricTraces)\Events Lost",
                    @"\Event Tracing for Windows Session(FabricTraces)\Events Logged per sec",
                    @"\Process(Fabric)\% Processor Time",
                    @"\Process(Fabric)\Private Bytes",
                    @"\Process(Fabric)\Thread Count",
                    @"\Process(Fabric)\Working Set",
                    @"\Process(Fabric_*)\% Processor Time",
                    @"\Process(Fabric_*)\Private Bytes",
                    @"\Process(Fabric_*)\Thread Count",
                    @"\Process(Fabric_*)\Working Set",
                    @"\Process(FabricDCA)\% Processor Time",
                    @"\Process(FabricDCA)\Private Bytes",
                    @"\Process(FabricDCA)\Thread Count",
                    @"\Process(FabricDCA)\Working Set",
                    @"\Process(FabricDCA_*)\% Processor Time",
                    @"\Process(FabricDCA_*)\Private Bytes",
                    @"\Process(FabricDCA_*)\Thread Count",
                    @"\Process(FabricDCA_*)\Working Set",
                    @"\Service Fabric Cluster Manager(*)\*",
                    @"\Service Fabric Naming Gateway(*)\*",
                    @"\Service Fabric Naming Service(*)\*",
                    @"\Service Fabric Health Manager Component(*)\*",
                    @"\Service Fabric Reconfiguration Agent(*)\*",
                    @"\Service Fabric Failover Manager Component(*)\*",
                    @"\Service Fabric File Store Service Component(*)\*",
                    @"\Service Fabric Image Builder(*)\*",
                    @"\Service Fabric Load Balancing Component(*)\*",
                    @"\Service Fabric Component JobQueue(*)\*",
                    @"\Service Fabric AsyncJobQueue(*)\*",
                    @"\Service Fabric Interlocked Bufferpool(*)\*",
                    @"\Service Fabric Transport(*)\*",
                    @"\PhysicalDisk(*)\% Disk Read Time",
                    @"\PhysicalDisk(*)\% Disk Time",
                    @"\PhysicalDisk(*)\% Disk Write Time",
                    @"\PhysicalDisk(*)\% Idle Time",
                    @"\PhysicalDisk(*)\Avg. Disk Bytes/Read",
                    @"\PhysicalDisk(*)\Avg. Disk Bytes/Write",
                    @"\PhysicalDisk(*)\Avg. Disk Read Queue Length",
                    @"\PhysicalDisk(*)\Avg. Disk Write Queue Length",
                    @"\PhysicalDisk(*)\Avg. Disk Queue Length",
                    @"\PhysicalDisk(*)\Disk Writes/sec",
                    @"\PhysicalDisk(*)\Disk Reads/sec",
                    @"\PhysicalDisk(*)\Disk Write Bytes/sec",
                    @"\PhysicalDisk(*)\Disk Read Bytes/sec",
                    @"\PhysicalDisk(*)\Avg. Disk sec/Read",
                    @"\PhysicalDisk(*)\Avg. Disk sec/Write",
                    @"\Memory\Available MBytes",
                    @"\Memory\Pool Nonpaged Bytes",
                    @"\Memory\Pool Paged Bytes",
                    @"\Memory\Pages/sec",
                    @"\Memory\Pages Input/sec",
                    @"\Memory\Pages Output/sec",
                    @"\Memory\Page Reads/sec",
                    @"\Ktl Log Manager(*)\Write Buffer Memory Pool Limit Bytes",
                    @"\Ktl Log Manager(*)\Write Buffer Memory Pool In Use Bytes",
                    @"\Ktl Log Container(*)\Shared Write Bytes/sec",
                    @"\Ktl Log Container(*)\Dedicated Write Bytes/sec",
                    @"\Paging File(*)\% Usage",
                } 
            },
            { 
                GroupPerPartition, 
                new[] 
                {
                    @"\Service Fabric Actor Method(*)\*",
                    @"\Service Fabric Actor(*)\*",
                } 
            },
            { 
                GroupPerReplica, 
                new[] 
                {
                    @"\Service Fabric Service Method(*)\*",
                    @"\Service Fabric Service(*)\*",
                    @"\Service Fabric Service Remoting(*)\*",
                    @"\Service Fabric Replicator(*)\*",
                    @"\Service Fabric Database ==> Instances(*)\*",
                    @"\Service Fabric ESE Local Store(*)\*",
                    @"\Service Fabric Replicated Store(*)\*",
                    @"\Service Fabric Transactional Replicator(*)\*",
                    @"\Service Fabric Native TransactionalReplicator(*)\*",
                    @"\Ktl Log Stream(*)\Application Write Bytes/sec",
                    @"\Ktl Log Stream(*)\Dedicated Write Bytes/sec",
                    @"\Ktl Log Stream(*)\Shared Write Bytes/sec",
                    @"\Ktl Log Stream(*)\Dedicated Log Backlog Bytes",
                    @"\Ktl Log Stream(*)\SharedLog Quota Bytes",
                    @"\Ktl Log Stream(*)\SharedLog In Use Bytes",
                    @"\Ktl Log Stream(*)\SharedLog Write Latency",
                    @"\Ktl Log Stream(*)\DedicatedLog Write Latency"                    
                } 
            },
            {
                GroupPerStore,
                new[]
                {
                    @"\Service Fabric Native Store(*)\*",
                    @"\Service Fabric TStore(*)\*"
                }
            }
        });

        internal static void ParseCounterList(string counterList, out HashSet<string> counterSet)
        {
            counterSet = new HashSet<string>();

            foreach (var rawToken in counterList.Split(','))
            {
                var token = rawToken.Trim();

                string[] groupsToAdd = null;

                if (CounterLabels.TryGetValue(token, out groupsToAdd))
                {
                    foreach (var group in groupsToAdd)
                    {
                        TryAddCountersFromGroup(group, counterSet);
                    }
                }
                else
                {
                    TryAddCountersFromGroup(token, counterSet);
                }
            }
        }

        private static void TryAddCountersFromGroup(string token, HashSet<string> counterSet)
        {
            string[] countersToAdd = null;

            if (CounterGroups.TryGetValue(token, out countersToAdd))
            {
                foreach (var counter in countersToAdd)
                {
                    counterSet.Add(counter);
                }
            }
            else
            {
                counterSet.Add(token);
            }
        }
    }
}