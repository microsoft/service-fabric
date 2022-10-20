// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;
    using EventsReader.LogProvider;
    using System.Fabric.Common.Tracing;
    using EventsReader.ConfigReader;
    using EventsReader.Exceptions;

    internal class EventsReaderExe
    {
        // Supported Events APIs

        // /EventsStore/Cluster/Events                                             (returns List<ClusterEvent>)
        // /EventsStore/Containers/Events                                          (returns List<ContainerEvent>)

        // /EventsStore/Nodes/Events                                               (returns List<NodeEvent>)
        // /EventsStore/Applications/Events                                        (returns List<ApplicationEvent>)
        // /EventsStore/Services/Events                                            (returns List<ServiceEvent>)
        // /EventsStore/Partitions/Events                                          (returns List<PartitionEvent>)
        // /EventsStore/Partitions/{partitionId}/$/Replicas/Events                 (returns List<ReplicaEvent>)          (partitionId is in uuid format)

        // /EventsStore/Nodes/{nodeName}/$/Events                                  (returns List<NodeEvent>)
        // /EventsStore/Applications/{applicationId}/$/Events                      (returns List<ApplicationEvent>)
        // /EventsStore/Services/{serviceId}/$/Events                              (returns List<ServiceEvent>)
        // /EventsStore/Partitions/{partitionId}/$/Events                          (returns List<PartitionEvent>)        (partitionId is in uuid format)
        // /EventsStore/Partitions/{partitionId}/$/Replicas/{replicaId}/$/Events   (returns List<ReplicaEvent>)          (partitionId is in uuid format)

        // Query parameters - Events APIs
        // api-version            (required, should be equal "6.2-preview")
        // StartTimeUtc           (required)
        // EndTimeUtc             (required)
        // EventsTypesFilter      (optional, comma separted list of Event types)
        // ExcludeAnalysisEvents  (optional, boolean to exclude any of AnalysisEvent types)
        // SkipCorrelationLookup  (optional, boolean to skip correlation search / set of HasCorrelatedEvents)

        // Supported Correlated Events APIs

        // /EventsStore/CorrelatedEvents/{eventInstanceId}/$/Events                (returns List<FabricEvent>)

        // Query parameters - Correlated Events APIs
        // api-version            (required, should be equal "6.2-preview")

        private static readonly HashSet<string> SupportedCommandArgs = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            ReaderConstants.OutputFile,
            ReaderConstants.GETQuery
        };

        /// <summary>
        /// </summary>
        /// <param name="args"></param>
        /// <returns>0 If successful</returns>
        public static int Main(string[] args)
        {
            //// Example args.
            //args = new string[] {
            //    "getQuery:EventsStore/Partitions/Events?api-version=6.2-preview&starttimeutc=2018-03-24T23:37:36Z&endtimeutc=2018-03-27T23:37:36Z",
            //     "outputFile:D:\\output.txt"
            //};

            if (args == null || args.Length < 2)
            {
                EventStoreLogger.Logger.LogWarning("Invalid Params");
                Console.WriteLine("This is for internal use only");
                return ErrorCodes.InvalidArgs;
            }

            /* Arguments */
            // /outputFile: Output file path (where serialized json result is written) - required
            // /getQuery: Events API GET query - required
            Dictionary<string, string> commandArgs = null;

            try
            {
                commandArgs = ParseParameters(args);
                if (!commandArgs.ContainsKey(ReaderConstants.OutputFile))
                {
                    EventStoreLogger.Logger.LogWarning("Params Missing output file");
                    return ErrorCodes.ParamMissingOutputFile;
                }

                var outputFileDirectory = Path.GetDirectoryName(commandArgs[ReaderConstants.OutputFile]);
                if (string.IsNullOrEmpty(outputFileDirectory) || !Directory.Exists(outputFileDirectory))
                {
                    EventStoreLogger.Logger.LogWarning("Param outputFileDirectory is empty, or Directory doesn't exist");
                    return ErrorCodes.ParamInvalidOutputDir;
                }

                if (!commandArgs.ContainsKey(ReaderConstants.GETQuery) || string.IsNullOrEmpty(commandArgs[ReaderConstants.GETQuery]))
                {
                    EventStoreLogger.Logger.LogWarning("getQuery required argument is missing");
                    return ErrorCodes.ParamMissingGetQueryKey;
                }

                // Unescape
                commandArgs[ReaderConstants.GETQuery] = Uri.UnescapeDataString(commandArgs[ReaderConstants.GETQuery].Trim());

                // Parse and forward to the router
                var parsedUri = RequestParser.ParseUri(commandArgs[ReaderConstants.GETQuery]);
                RequestRouter.RouteUri(parsedUri, new FileResultWriter(commandArgs[ReaderConstants.OutputFile]));
            }
            catch (Exception e)
            {
                FabricEvents.Events.EventStoreFailed(string.Join(",", args), e.ToString());
                try
                {
                    WriteToFile(
                        string.Concat(
                            commandArgs[ReaderConstants.OutputFile],
                            ReaderConstants.ErrorFileExt),
                        e.ToString());
                }
                catch
                {
                    // Nothing to do further.
                }

                return GetErrorCode(e);
            }

            return 0;
        }

        private static Dictionary<string, string> ParseParameters(string[] args)
        {
            var commandArgs = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (var arg in args)
            {
                var delimiterIndex = arg.IndexOf(":", StringComparison.OrdinalIgnoreCase);

                if (delimiterIndex == -1)
                {
                    throw new ArgumentException(
                        string.Format(
                            "Invalid argument {0}",
                            arg));
                }

                var key = arg.Substring(0, delimiterIndex);
                var value = arg.Substring(delimiterIndex + 1).Trim();

                if (commandArgs.ContainsKey(key) || !SupportedCommandArgs.Contains(key))
                {
                    continue;
                }

                commandArgs.Add(key, value);
            }

            return commandArgs;
        }

        private static void WriteToFile(string filePath, string data)
        {
            // Write in UTF8 including BOM
            File.WriteAllText(filePath, data, Encoding.UTF8);
        }
        
        private static int GetErrorCode(Exception exp)
        {
            var evtException = exp as EventStoreException;
            if (evtException != null)
            {
                return evtException.ErrorCode;
            }

            if(exp is ArgumentException || exp is ArgumentNullException)
            {
                return ErrorCodes.GenericArgumentException;
            }

            return ErrorCodes.GenericError;
        }
        //
        // Simple output writer
        //
        private class FileResultWriter : IResultWriter
        {
            private string path;

            public FileResultWriter(string path)
            {
                this.path = path;
            }

            public void Write(string serializedData)
            {
                // Write in UTF8 including BOM
                File.WriteAllText(this.path, serializedData, Encoding.UTF8);
            }
        }
    }
}