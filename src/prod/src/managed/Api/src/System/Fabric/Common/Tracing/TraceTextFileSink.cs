// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;

    /// <summary>
    /// Sink that writes trace records to text file.
    /// </summary>
    internal static class TraceTextFileSink
    {
        private const int MaxFilesToKeep = 3;
        private const string Extension = ".trace";

        private static readonly object SyncLock = new object();
        
        private static TextWriter writer = null;
        private static bool isEnabled = false;
        private static string path = null;
        private static string option = string.Empty;
        private static DateTime segmentTime;
        private static List<string> files = new List<string>();

        internal static bool IsEnabled
        {
            get
            {
                return isEnabled;
            }
        }

        /// <summary>
        /// Set the path of the text file sink.
        /// Before the path is set, the sink is effectively disabled.
        /// </summary>
        /// <param name="path">Path for the text file where the records are written to.</param>
        public static void SetPath(string path)
        {
            lock (TraceTextFileSink.SyncLock)
            {
                if (TraceTextFileSink.path != path)
                {
                    TraceTextFileSink.path = path;
                    Close();

                    isEnabled = !string.IsNullOrEmpty(path);
                }
            }
        }

        /// <summary>
        /// Set option for text file sink.
        /// </summary>
        /// <param name="option">If 'h' is contained, trace file is truncated
        /// every hour.
        /// If 'p' is contained, process id is appended to the file name.
        /// </param>
        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public static void SetOption(string option)
        {
            lock (TraceTextFileSink.SyncLock)
            {
                if (TraceTextFileSink.option != option)
                {
                    TraceTextFileSink.option = option;
                    Close();
                }
            }
        }

        internal static void Write(string taskName, string eventName, string id, EventLevel level, string text)
        {
            StringBuilder output = new StringBuilder(text.Length + 128);

            DateTime now = DateTime.UtcNow;
            output.Append(now.ToString("yyyy/MM/dd-HH:mm:ss.fff", CultureInfo.InvariantCulture));
            output.Append(',');
            output.Append(ConvertLevelToString(level));
            output.Append(',');
#if DotNetCoreClrLinux
            //To Do: Fix GetCurrentThreadId
            output.Append(0);
#else
            output.Append(GetCurrentThreadId());
#endif
            output.Append(',');
            output.Append(taskName);

            if (!string.IsNullOrEmpty(eventName))
            {
                output.Append('.');
                output.Append(eventName);
            }

            if (!string.IsNullOrEmpty(id))
            {
                output.Append('@');
                output.Append(id);
            }

            output.Append(',');
            output.Append(text.Replace('\n', '\t'));

            lock (SyncLock)
            {
                if (now > segmentTime)
                {
                    Close();
                }

                if (writer == null)
                {
                    Open();
                }

                writer.WriteLine(output.ToString());
                writer.Flush();
            }
        }

        private static string ConvertLevelToString(EventLevel level)
        {
            switch (level)
            {
                case EventLevel.Informational:
                    return "Info";
                default:
                    return level.ToString();
            }
        }

        private static void Close()
        {
            if (writer != null)
            {
                writer.Dispose();
                writer = null;
            }
        }

        private static void CalculateSegmentTime(DateTime now)
        {
            long ticks = (now + TimeSpan.FromHours(1)).Ticks;
            ticks -= ticks % TimeSpan.TicksPerHour;

            segmentTime = new DateTime(ticks);
        }

        private static void Open()
        {
            string fileName;
            if (path.EndsWith(Extension, StringComparison.OrdinalIgnoreCase))
            {
                fileName = path.Substring(0, path.Length - Extension.Length);
            }
            else
            {
                fileName = path;
            }

            if (option.IndexOf('p') >= 0)
            {
                fileName += Process.GetCurrentProcess().Id.ToString(CultureInfo.InvariantCulture);
            }

            DateTime now = DateTime.Now;
            if (option.IndexOf('h') >= 0)
            {
                CalculateSegmentTime(now);
                fileName += "-" + now.Hour.ToString(CultureInfo.InvariantCulture);
            }
            else
            {
                segmentTime = DateTime.MaxValue;
            }

            fileName += Extension;

            writer = new StreamWriter(File.Open(fileName, FileMode.Create, FileAccess.Write, FileShare.ReadWrite));

            files.Add(fileName);
            if (files.Count > MaxFilesToKeep)
            {
                try
                {
                    File.Delete(files[0]);
                }
                catch (IOException)
                {
                }

                files.RemoveAt(0);
            }
        }

        [DllImport("KERNEL32.dll", ExactSpelling = true, SetLastError = true)]
        private static extern uint GetCurrentThreadId();
    }
}