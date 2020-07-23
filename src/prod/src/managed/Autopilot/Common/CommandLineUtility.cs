// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{   
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;

    internal static class CommandLineUtility
    {
        private static int DefaultCommandExecutionTimeoutSeconds = 300;

        public static int ExecuteNetshCommand(string arguments)
        {
            return ExecuteCommand("netsh", arguments);
        }

        public static int ExecuteCommand(string command, string arguments)
        {
            return ExecuteCommand(command, arguments, TimeSpan.FromSeconds(DefaultCommandExecutionTimeoutSeconds));
        }

        public static int ExecuteCommand(string command, string arguments, TimeSpan timeout)
        {
            return ExecuteCommand(command, arguments, Directory.GetCurrentDirectory(), timeout);
        }

        public static int ExecuteCommand(string command, string arguments, string workingDirectory, TimeSpan timeout)
        {
            TextLogger.LogInfo("Executing command {0} with arguments = {1}, working directory = {2}, timeout = {3} seconds.", command, arguments, workingDirectory, timeout == TimeSpan.MaxValue ? "infinite" : timeout.TotalSeconds.ToString());

            using (Process process = new Process())
            {
                process.StartInfo.FileName = command;
                process.StartInfo.Arguments = arguments;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.WorkingDirectory = workingDirectory;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;

                ProcessStandardOutputHandler processStandardOutputHandler = new ProcessStandardOutputHandler();
                process.OutputDataReceived += new DataReceivedEventHandler(processStandardOutputHandler.StandardOutputHandler);
                process.ErrorDataReceived += new DataReceivedEventHandler(processStandardOutputHandler.StandardErrorHandler);

                process.Start();

                process.BeginOutputReadLine();

                process.BeginErrorReadLine();

                int exitCode;
                if (timeout == TimeSpan.MaxValue)
                {
                    process.WaitForExit();

                    TextLogger.LogInfo("Commond execution completed with exit code {0}.", process.ExitCode);

                    exitCode = process.ExitCode;
                }
                else
                {
                    if (!process.WaitForExit((int)timeout.TotalMilliseconds))
                    {
                        process.Kill();

                        TextLogger.LogError("Execution of command {0} with arguments {1} timed out after {2} seconds. The process is terminated.", command, arguments, timeout.TotalSeconds);

                        exitCode = -1;
                    }
                    else
                    {
                        TextLogger.LogInfo("Commond execution completed with exit code {0}.", process.ExitCode);

                        exitCode = process.ExitCode;
                    }
                }                

                if (!string.IsNullOrEmpty(processStandardOutputHandler.StandardOutput))
                {
                    TextLogger.LogInfo("Standard output : {0}", processStandardOutputHandler.StandardOutput);
                }

                if (!string.IsNullOrEmpty(processStandardOutputHandler.StandardError))
                {
                    TextLogger.LogInfo("Standard error : {0}", processStandardOutputHandler.StandardError);
                }

                return exitCode;
            }
        }

    }
}