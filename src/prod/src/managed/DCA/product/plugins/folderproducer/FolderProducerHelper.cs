// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.IO;

    // Class that implements helper functions for foler producer.
    internal class FolderProducerHelper
    {
#if DotNetCoreClrLinux
        internal static string GetKernelCrashFolder(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            string string_to_check = "Not ready to kdump";
            string kdump_config_output = string.Empty;
            string kernelCrashFolderPath = string.Empty;

            try
            {
                using (var p = new Process())
                {
                    var startInfo = new ProcessStartInfo { FileName = "kdump-config", Arguments = "status" };
                    startInfo.RedirectStandardOutput = true;
                    p.StartInfo = startInfo;
                    p.Start();
                    p.WaitForExit(3000);

                    using (var reader = p.StandardOutput)
                        kdump_config_output = reader.ReadToEnd();
                }

                if (kdump_config_output == null)
                {
                    traceSource.WriteWarning(
                        logSourceId,
                        "kdump status output empty");
                    return string.Empty;
                }
            }
            catch (Exception e)
            {
                traceSource.WriteExceptionAsWarning(
                    logSourceId,
                    e);
                return string.Empty;
            }

            if (kdump_config_output.Contains(string_to_check))
            {
                traceSource.WriteWarning(
                        logSourceId,
                        "kdump is not configured");
                return string.Empty;
            }

            try
            {
                using (var p = new Process())
                {
                    var startInfo = new ProcessStartInfo { FileName = "/bin/bash", Arguments = "-c \" kdump-config show | grep \"KDUMP_COREDIR:\" | cut -f 5 -d' ' \"   " };
                    startInfo.RedirectStandardOutput = true;
                    p.StartInfo = startInfo;
                    p.Start();
                    p.WaitForExit(3000);

                    using (var reader = p.StandardOutput)
                    {
                        kdump_config_output = reader.ReadToEnd();
                    }
                }

                if (kdump_config_output == null)
                {
                    traceSource.WriteWarning(
                        logSourceId,
                        "kdump config output empty");
                    return string.Empty;
                }

                kernelCrashFolderPath = Path.GetFullPath(kdump_config_output);
            }
            catch (Exception e)
            {
                traceSource.WriteExceptionAsWarning(
                    logSourceId,
                    e);
                return string.Empty;
            }
            
            return kernelCrashFolderPath.Trim();
        }
#else
        internal static string GetKernelCrashFolder(FabricEvents.ExtensionsEvents traceSource, string logSourceId)
        {
            return string.Empty;
        }
#endif
    }
}