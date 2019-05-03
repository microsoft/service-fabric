//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin
{
    using System;
    using System.Fabric;
    using Microsoft.ServiceFabric.Diagnostics.Tracing;

    internal static class TraceWriter
    {
        private static bool writeToConsole;

        static TraceWriter()
        {
            var os = Utilities.GetOperatingSystem(FabricRuntime.GetActivationContext());
            writeToConsole = (os == AzureFilesVolumePluginSupportedOs.Windows)
                && (!ServiceFabricStringEventSource.Instance.IsEnabled());
        }

        internal static void WriteError(string type, string format, params object[] args)
        {
            WriteErrorWithId(type, string.Empty, format, args);
        }

        internal static void WriteErrorWithId(string type, string id, string format, params object[] args)
        {
            if (writeToConsole)
            {
                WriteToConsole(type, id, format, args);
            }
            else
            {
                ServiceFabricStringEventSource.Instance.WriteErrorWithId(type, id, format, args);
            }
        }

        internal static void WriteWarning(string type, string format, params object[] args)
        {
            WriteWarningWithId(type, string.Empty, format, args);
        }

        internal static void WriteWarningWithId(string type, string id, string format, params object[] args)
        {
            if (writeToConsole)
            {
                WriteToConsole(type, id, format, args);
            }
            else
            {
                ServiceFabricStringEventSource.Instance.WriteWarningWithId(type, id, format, args);
            }
        }

        internal static void WriteInfo(string type, string format, params object[] args)
        {
            WriteInfoWithId(type, string.Empty, format, args);
        }

        internal static void WriteInfoWithId(string type, string id, string format, params object[] args)
        {
            if (writeToConsole)
            {
                WriteToConsole(type, id, format, args);
            }
            else
            {
                ServiceFabricStringEventSource.Instance.WriteInfoWithId(type, id, format, args);
            }
        }

        internal static void WriteNoise(string type, string format, params object[] args)
        {
            WriteNoiseWithId(type, string.Empty, format, args);
        }

        internal static void WriteNoiseWithId(string type, string id, string format, params object[] args)
        {
            if (writeToConsole)
            {
                WriteToConsole(type, id, format, args);
            }
            else
            {
                ServiceFabricStringEventSource.Instance.WriteNoiseWithId(type, id, format, args);
            }
        }

        private static void WriteToConsole(string type, string id, string format, params object[] args)
        {
            Console.WriteLine(
                "{0} - {1}{2} - {3}",
                DateTime.UtcNow.ToString("MM/dd/yyyy HH:mm:ss.fff"),
                type,
                String.IsNullOrEmpty(id) ? String.Empty : String.Concat("@", id),
                String.Format(format, args));
        }
    }
}
