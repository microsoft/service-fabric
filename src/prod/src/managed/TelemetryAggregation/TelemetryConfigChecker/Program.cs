// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryConfigParser
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using TelemetryAggregation;

    public class Program
    {
        public static int Main(string[] args)
        {
            if (args.Count() < 1)
            {
                Console.WriteLine("Usage: TelemetryConfigChecker.exe <Configfile path> [-v] [<manifest file path 1> <manifest file path 2> ... ]");

                return 1;
            }

            string filePath = args[0];

            // simple parse of the arguments (looks for -v on the second argument)
            bool verbose = (args.Count() > 1) ? ((args.ElementAt(1) == "-v") ? true : false) : false;

            IEnumerable<string> manifestFilePaths = args.Skip(verbose ? 2 : 1).Take(args.Length - 1);

            List<TraceAggregationConfig> telemetryConfigEntryList;

            // parsing the config file
            List<ParseResult> parseResults = SyntaxAnalyzer.Parse(filePath, manifestFilePaths, (type, format, arg) => { Console.WriteLine(type + ": " + format, arg); }, verbose, out telemetryConfigEntryList);

            if (verbose)
            {
                // printing the configurations loaded
                foreach (var telConfigEntry in telemetryConfigEntryList)
                {
                    Console.WriteLine(telConfigEntry.ToString());
                }
            }

            Console.WriteLine("#Configs Lines sucessfuly parsed   = {0}", telemetryConfigEntryList.Count);
            Console.WriteLine("#Configs Lines failed to be parsed = {0}", parseResults.Count - telemetryConfigEntryList.Count);

            if (telemetryConfigEntryList.Count != parseResults.Count)
            {
                Console.WriteLine("Parsing Failed");

                // printing the errors occurred
                foreach (var parseRes in parseResults)
                {
                    if (parseRes.ParseCode != ParseCode.Success)
                    {
                        Console.WriteLine("{0}({1},{2}) : Error : {3}", filePath, parseRes.LineNumber, parseRes.ColumnPos + 1, parseRes.ErrorMsg);
                    }
                }
            }
            else
            {
                Console.Write("Parsing Succeeded");
            }

            return 0;
        }
    }
}