// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace KtlTruncateTail
{
    using System;
    using System.IO;
    using System.Collections.Generic;
    using System.Threading;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using System.Runtime.InteropServices;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Test;
    using System.Linq;
    using System.Security.AccessControl;
    using System.Fabric.Data.Log;
    using System.Fabric.Data.Log.Interop;
    using System.Runtime.CompilerServices;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Xml;


    class Program
    {
        static int Main(string[] args)
        {
            Task<int> resultTask;
            Program program = new Program();

            resultTask = program.TruncateTailTool(args);
            resultTask.Wait();

            return (resultTask.Result);
        }

        internal string PhysicalLogName;
        internal Guid PhysicalLogId;
        internal Guid LogicalLogId;
        internal long TruncateTailOffset;
#if DotNetCoreClrLinux
        private static readonly string PlatformPathPrefix = "";
#else
        private static readonly string PlatformPathPrefix = "\\??\\";
#endif

        void Usage()
        {
            Console.WriteLine("KtlTruncateTail - manually truncate tail for a specific log stream");
            Console.WriteLine("");
            Console.WriteLine("******** WARNING: Incorrect usage of this tool WILL CAUSE DATA LOSS ********");
            Console.WriteLine("********          If you do not know what truncate tail means then  ********");
            Console.WriteLine("********          you should not be using this tool. If you do not  ********");
            Console.WriteLine("********          know what offset to truncate tail then you should ********");
            Console.WriteLine("********          not be using this tool.                           ********");
            Console.WriteLine("");
            Console.WriteLine("    -l:<filename of log container/shared log>");
            Console.WriteLine("    -g:<Shared log Guid>");
            Console.WriteLine("    -s:<stream id for stream to truncate>");
            Console.WriteLine("    -r:<stream offset at which to truncate tail>");
            Console.WriteLine("");
            Console.WriteLine("NOTE: Default value for -g is {3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62} which is default shared log id");
            Console.WriteLine("");
            Console.WriteLine("Example: Truncate tail at a specific stream offset:");
            Console.WriteLine("         KtlTruncateTail -l:c:\\ReplicatorShared\\Replicator.Log -g:{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62} -s:{F96E0B9A-1055-499e-BA5A-15C404AC3665} -r:12388904");
            Console.WriteLine("         KtlTruncateTail -l:c:\\ReplicatorShared\\Replicator.Log -s:{F96E0B9A-1055-499e-BA5A-15C404AC3665} -r:12388904");
            Console.WriteLine("");
            Console.WriteLine("Example: Retrieve current tail truncation location:");
            Console.WriteLine("         KtlTruncateTail -l:c:\\ReplicatorShared\\Replicator.Log -g:{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62} -s:{F96E0B9A-1055-499e-BA5A-15C404AC3665}");
        }

        bool ParseCommandLineArgs(string[] args)
        {
            string defaultSharedLogId = "{3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62}";
            PhysicalLogId = Guid.Parse(defaultSharedLogId);

            PhysicalLogName = "";

            TruncateTailOffset = -1;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i].Length < 3)
                {
                    return(false);
                }

                if (args[i][0] != '-')
                {
                    return (false);
                }

                if (args[i][2] != ':')
                {
                    return (false);
                }

                char c = args[i][1];

                if ((c == 'L' || c== 'l'))
                {
                    PhysicalLogName = PlatformPathPrefix + args[i].Substring(3);
                }
                else if ((c == 'G') || (c == 'g'))
                {
                    try
                    {
                        PhysicalLogId = Guid.Parse(args[i].Substring(3));
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("Exception {0} parsing PhysicalLogId {1}", ex.Message, args[i].Substring(3));
                        Console.WriteLine("");
                        return (false);
                    }
                }
                else if ((c == 'S') || (c == 's'))
                {
                    try
                    {
                        LogicalLogId = Guid.Parse(args[i].Substring(3));
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("Exception {0} parsing LogicalLogId {1}", ex.Message, args[i].Substring(3));
                        Console.WriteLine("");
                        return (false);
                    }
                } 
                else if ((c == 'R') || (c == 'r'))
                {
                    try
                    {
                        TruncateTailOffset = long.Parse(args[i].Substring(3));
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("Exception {0} parsing TruncateTailOffset {1}", ex.Message, args[i].Substring(3));
                        Console.WriteLine("");
                        return (false);
                    }
                }
            }

            if (PhysicalLogName == "")
            {
                return (false);
            }

            return(true);
        }

        async Task<int> TruncateTailTool(string[] args)
        {
            try
            {
                bool parsedOk;

                parsedOk = ParseCommandLineArgs(args);

                if (!parsedOk)
                {
                    Usage();
                    return (-1);
                }

                ILogManager logManager;
                IPhysicalLog logContainer;
                ILogicalLog logStream;

                logManager = await CreateLogManager();
                logContainer = await OpenPhysicalLog(logManager, PhysicalLogName, PhysicalLogId);
                logStream = await OpenLogicalLog(logContainer, LogicalLogId);

                Console.WriteLine("Current tail offset is {0}", logStream.Length);

                if (TruncateTailOffset != -1)
                {
                    if (TruncateTailOffset >= logStream.Length)
                    {
                        Console.WriteLine("FAILED: Truncate tail offset {0} is beyond the end of the stream {1}", TruncateTailOffset, logStream.Length);
                    } else {
                        TruncateTail(logStream, TruncateTailOffset);

                        await logStream.CloseAsync(CancellationToken.None);

                        logStream = await OpenLogicalLog(logContainer, LogicalLogId);

                        Console.WriteLine("New tail offset is {0}", logStream.Length);
                    }
                }

                await logStream.CloseAsync(CancellationToken.None);
                await logContainer.CloseAsync(CancellationToken.None);
                await logManager.CloseAsync(CancellationToken.None);

                return (0);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return (-3);
            }
        }
    
        async Task<ILogManager> CreateLogManager()
        {
            ILogManager t = await LogManager.OpenAsync(LogManager.LoggerType.Default, CancellationToken.None);
            return(t);
        }

        async Task<IPhysicalLog> OpenPhysicalLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId)
        {
            IPhysicalLog t = await manager.OpenPhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                false,
                CancellationToken.None);

            return t;
        }

        async Task<ILogicalLog> OpenLogicalLog(
            IPhysicalLog phylog,
            Guid logicalLogId)
        {
            ILogicalLog t = await phylog.OpenLogicalLogAsync(
                logicalLogId,
                "",
                CancellationToken.None);

            return t;
        }

        /// <summary>
        /// Truncate the most recent information
        /// </summary>
        async void TruncateTail(
            ILogicalLog logicalLog,
            long streamOffset)
        {
            await logicalLog.TruncateTail(streamOffset, CancellationToken.None);
        }
    }
}