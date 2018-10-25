// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.Tracing;

    internal static class TraceConsoleSink
    {
        private static readonly object SyncLock = new object();

        public static void Write(EventLevel level, string text)
        {
            lock (TraceConsoleSink.SyncLock)
            {
                if (level >= EventLevel.Informational)
                {
                    Console.WriteLine(text);
                }
                else
                {
                    ConsoleColor oldColor = Console.ForegroundColor;

                    Console.ForegroundColor = level == EventLevel.Warning ? ConsoleColor.Yellow : ConsoleColor.Red;
                    Console.WriteLine(text);

                    Console.ForegroundColor = oldColor;
                }
            }
        }
    }
}