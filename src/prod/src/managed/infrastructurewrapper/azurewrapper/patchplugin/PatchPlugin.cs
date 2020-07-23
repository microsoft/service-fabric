// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Threading;
using System.Management;

namespace Microsoft.Xdb.PatchPlugin
{
    public class PatchPlugin
    {
        private static int Main(string[] args)
        {
            bool success = false;

            LogUtils.WriteTrace(DateTime.UtcNow, "patch plugin started");
            // Run patches
            try
            {
                success = RunTasks();
            }
            catch (Exception ex)
            {
                LogUtils.WriteTrace(DateTime.UtcNow, "Sterling patch operation ran into Exception: " + ex);
            }

            return (success ? 0 : 1);
        }

        private static bool RunTasks()
        {
            // Perform OS patch check & installation
            PatchUtil patchUtil = new PatchUtil();
            patchUtil.InstallPatches();

            // Reboot if needed
            if (patchUtil.RebootRequired)
            {
                Reboot();
            }

            return true;
        }

        /// <summary>
        /// This function reboots the local machine using the Win32_OperatingSystem 
        /// WMI object from the local WMI instance
        /// </summary>
        internal static void Reboot()
        {
            try
            {
                string computerName = Environment.MachineName; // computer name or IP address

                ConnectionOptions options = new ConnectionOptions();
                options.EnablePrivileges = true;
                // To connect to the remote computer using a different account, specify these values:
                // options.Username = "USERNAME";
                
                // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Comment with example")]
                // options.Password = "PASSWORD";
                // options.Authority = "ntlmdomain:DOMAIN";

                ManagementScope scope = new ManagementScope("\\\\" + computerName + "\\root\\CIMV2", options);
                scope.Connect();

                SelectQuery query = new SelectQuery("Win32_OperatingSystem");
                ManagementObjectSearcher searcher = new ManagementObjectSearcher(scope, query);

                LogUtils.WriteTrace(DateTime.UtcNow, "Requesting machine reboot");

                // DEV NOTE : We are missing adding UploadData to Xstore here as I did not want that to interrupt this work
                // UploadData();

                foreach (ManagementObject os in searcher.Get())
                {
                    // Obtain in-parameters for the method
                    ManagementBaseObject inParams = os.GetMethodParameters("Win32Shutdown");

                    // Add the input parameters for Forced Reboot 
                    inParams["Flags"] = 6;

                    // Execute the method and obtain the return values.
                    os.InvokeMethod("Win32Shutdown", inParams, null);
                }

                // Halt execution permanently; machine should reboot
                Halt();
            }
            catch (ManagementException err)
            {
                LogUtils.WriteTrace(DateTime.UtcNow, string.Format("An error occurred while trying to execute the WMI method: {0} ", err));
                throw;
            }
            catch (UnauthorizedAccessException unauthorizedErr)
            {
                LogUtils.WriteTrace(DateTime.UtcNow, string.Format("Connection error (user name or password might be incorrect): {0} ", unauthorizedErr));
                throw;
            }
        }

        private static void Halt()
        {
            while (true)
            {
                Thread.Sleep(TimeSpan.FromMinutes(1));
                LogUtils.WriteTrace(DateTime.UtcNow, "Waiting for reboot to occur...");
            }
            // ReSharper disable FunctionNeverReturns
        }
        // ReSharper restore FunctionNeverReturns
    }

}