// ----------------------------------------------------------------------
//  <copyright file="Program.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;

    // Handy Util to run the code as a program. Not built by default.
    class Program
    {
        static void Main(string[] args)
        {
            //TraceStoreConnectionInformation info = new LocalTraceStoreConnectionInformation(
            //    @"D:\Repos\WindowsFabric\out\debug-amd64\bin\WinFabricTest\test\FabActFiveNodelCluster.test\TC",
            //    @"E:\crm_duplicate",
            //    @"D:\Repos\WindowsFabric\out\debug-amd64\bin\WinFabricTest\test\FabActFiveNodelCluster.test\TC");

            TraceStoreConnectionInformation info = new AzureTraceStoreConnectionInformation(
                "winfablrc",
                 HandyUtil.SecureStringFromCharArray("<Insert key Here>"),
                 "vipinrtobitlogsqt",
                 "blobContainerRandmom",
                 "e98a88aa63aa42b4",
                 LocalDiskLogger.LogProvider);

            TraceStoreConnection connection = new TraceStoreConnection(info, LocalDiskLogger.LogProvider);
            ReadFilter filter = ReadFilter.CreateReadFilter(typeof(NodeOpeningTraceRecord));

            // SET your Duration.
            var duration = new Duration(
                DateTime.SpecifyKind(DateTime.Parse("2018-04-10 01:01:19.155"), DateTimeKind.Utc),
                DateTime.SpecifyKind(DateTime.UtcNow, DateTimeKind.Utc));

            var results = connection.EventStoreReader.WithinBound(duration).ReadBackwardAsync(filter, 500, CancellationToken.None).GetAwaiter().GetResult();

            Console.WriteLine("Count {0}", results.Count());
            foreach (var one in results)
            {
                Console.WriteLine(one.TimeStamp.ToString("yyyy-MM-dd HH:mm:ss.fff", CultureInfo.InvariantCulture) + " " + one);
                Console.WriteLine();
            }

            Console.Read();
        }
    }
}