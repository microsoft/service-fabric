// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.Globalization;

    internal class Logger
    {
        public static void LogInfo(string type, string format, params object[] args)
        {
            var outputFormat = string.Format(CultureInfo.InvariantCulture, "Info: {0}:- {1}", type, format);
            Console.WriteLine(outputFormat, args);
        }

        public static void LogError(string type, string format, params object[] args)
        {
            var outputFormat = string.Format(CultureInfo.InvariantCulture, "Error: {0}:- {1}", type, format);
            Console.WriteLine(outputFormat, args);
        }

        public static void LogWarning(string type, string format, params object[] args)
        {
            var outputFormat = string.Format(CultureInfo.InvariantCulture, "Warning: {0}:- {1}", type, format);
            Console.WriteLine(outputFormat, args);
        }

        public static void LogFatalException(string type, Exception e)
        {
            Console.WriteLine("Exception: {0}:- {1}", type, e);
        }
    }
}