// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Reader;
    using Readers.AzureTableReader;
    using Definitions.TypedTraceRecords.FM;
    using Definitions.TypedTraceRecords.RA;
    using System.Linq;

    // Just for Illustration on how the experience is. This doesn't get built.
    class Program
    {
        private const string DateTimeFormat = "yyyy MM dd HH:mm:ss.fffffff";

        private static TraceRecordSession azureSession;

        public static IList<TraceRecord> collect = new List<TraceRecord>();

        // public static IDisposable dispose;

        static void Main(string[] args)
        {
            //azureSession = new ServiceFabricAzureTableRecordSession(
            //    "winfablrc",
            //    "<key>",
            //    true,
            //    "vipinchaos1logsqt",
            //    null);

            var files = Directory.EnumerateFiles(@"E:\crm_duplicate\OperationalTraces\", "*.etl" );

            azureSession = new EtwTraceRecordSession(files.ToList());

            var parser = new EventStoreTraceRecordParser(azureSession);

            parser.SubscribeAsync(typeof(ReconfigurationCompletedTraceRecord), Parser_HandleRecord, CancellationToken.None).GetAwaiter().GetResult();
            parser.SubscribeAsync(typeof(NodeOpenedFailedTraceRecord), Parser_HandleRecord, CancellationToken.None).GetAwaiter().GetResult();

            azureSession.StartReadingAsync(DateTimeOffset.UtcNow - TimeSpan.FromDays(500), DateTimeOffset.UtcNow, CancellationToken.None).GetAwaiter().GetResult();

            Console.WriteLine("Record Count " + collect.Count);

            Console.Read();
        }

        private static Task Parser_HandleRecord(TraceRecord obj, CancellationToken token)
        {
            collect.Add(obj);

            Console.WriteLine("Type : {0}. Value: {1}", obj.GetType().Name, obj.ToString());

            Console.WriteLine("--------                ----");

            return Task.FromResult(true);
        }


        /// <summary>
        /// Serialize a given object to string using JSON serializer
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="objectToBeSerialized"></param>
        /// <returns></returns>
        internal static string Serialize<T>(T objectToBeSerialized)
        {
            string serializedProcessCrashDetails;
            var serializer = new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { DateTimeFormat = new DateTimeFormat(DateTimeFormat), EmitTypeInformation = EmitTypeInformation.AsNeeded });

            using (var ms = new MemoryStream())
            {
                serializer.WriteObject(ms, objectToBeSerialized);

                // TODO: Investigate why UTF16 is causing crashes and then switch to it
                serializedProcessCrashDetails = Encoding.UTF8.GetString(ms.ToArray());
            }

            return serializedProcessCrashDetails;
        }

        /// <summary>
        /// Deserialize given string to object to requested type
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="jsonSerializedStr"></param>
        /// <returns></returns>
        internal static T DeSerialize<T>(string jsonSerializedStr)
        {
            using (var ms = new MemoryStream(Encoding.UTF8.GetBytes(jsonSerializedStr)))
            {
                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(T), new DataContractJsonSerializerSettings { DateTimeFormat = new DateTimeFormat(DateTimeFormat), EmitTypeInformation = EmitTypeInformation.AsNeeded });
                return (T)deserializer.ReadObject(ms);
            }
        }
    }
}