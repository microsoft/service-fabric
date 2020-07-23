// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FileLogUtility
{
    using System;

    public class Program
    {
        public static int Main(string[] args)
        {
            if (args.Length < 1 || args.Length > 2)
            {
                return PrintUsage();
            }

            try
            {
                // Parse commandline arguments.
                string logPath = args[0];
                bool isPhysical = (args.Length > 1 ? bool.Parse(args[1]) : true);

                // Process the FileLogicalLog.
                try
                {
                    Console.WriteLine("Parsing {0} ...", logPath);

                    FileLogicalLogParser parser = new FileLogicalLogParser(logPath, isPhysical);
                    parser.ParseAsync().Wait();
                    parser.DisplayStatistics();
                }
                catch (Exception e)
                {
                    Console.WriteLine();
                    Console.WriteLine("Unexpected exception: {0}", e);
                    Console.WriteLine();
                }
            }
            catch (Exception e)
            {
                Console.WriteLine();
                Console.WriteLine("Unexpected exception parsing commandline arguments: {0}", e);
                Console.WriteLine();

                return PrintUsage();
            }

            return 0;
        }

        private static int PrintUsage()
        {
            Console.WriteLine("usage:");
            Console.WriteLine("  FileLogUtil.exe <path to a FileLogicalLog> [physical/logical record: true/false]");
            Console.WriteLine();

            return 1;
        }
    }
}