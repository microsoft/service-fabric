// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.IO;
using System.Threading;

namespace Microsoft.Xdb.PatchPlugin
{
	internal class ScriptingHelper
	{
		internal class ScriptExecution
		{
			private readonly TextWriter _out;
			private readonly TextWriter _err;
			private readonly object _outputLock = new object();
			private readonly object _errorLock = new object();
			private bool _disableDataHandlers;

			/// <summary>
			/// Disables DataHandlers.
			/// If true, output is not written to the streams.
			/// This is useful when streams are disposed / not available.
			/// </summary>
			internal bool DisableDataHandlers
			{
				get
				{
					return _disableDataHandlers;
				}

				set
				{
					// don't stack (outputLock & errorLock) but call separately as don't know how Process class works with handlers so to not cause deadlock
					lock (_outputLock)
					{
						_disableDataHandlers = true;
					}

					lock (_errorLock)
					{
						// again reassign the _disableDataHandlers (so we call smth within the lock section) so compiler doesn't remove the empty lock section
						_disableDataHandlers = true;
					}
				}
			}

			internal ScriptExecution(TextWriter outStream, TextWriter errStream)
			{
				_out = outStream;
				_err = errStream;
			}

			internal void OutputDataHandler(object sendingProcess, DataReceivedEventArgs outLine)
			{
				lock (_outputLock)
				{
					if (_out != null && outLine.Data != null && !_disableDataHandlers)
					{
						_out.WriteLine(outLine.Data);
					}
				}
			}

			internal void ErrorDataHandler(object sendingProcess, DataReceivedEventArgs outLine)
			{
				lock (_errorLock)
				{
					if (_err != null && outLine.Data != null && !_disableDataHandlers)
					{
						_err.WriteLine(outLine.Data);
					}
				}
			}
		}

		/// <summary>
		/// An overloaded execute command to execute a command with no out and err redirection.
		/// </summary>
		/// <param name="command">
		/// The command to be executed.
		/// </param>
		/// <param name="wait">
		/// True if the function should wait for the child process to finish before returning, false otherwise.
		/// </param>
		/// <returns>
		/// The exit code of the process executed.
		/// </returns>
		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		public static int Execute(string command, bool wait)
		{
			int returnCode = 0;
			using (Process proc = new Process())
			{
				proc.EnableRaisingEvents = true;
				proc.StartInfo.Arguments = "/c " + command + string.Empty;
				proc.StartInfo.FileName = "cmd.exe";
				proc.StartInfo.UseShellExecute = false;
				proc.StartInfo.CreateNoWindow = true;
				proc.Start();
				if (wait)
				{
					proc.WaitForExit();
					returnCode = proc.ExitCode;
				}
				else
				{
					returnCode = 0;
				}
			}

			return returnCode;
		}

		/// <summary>
		/// This method executes a particular command. It checks if the program has not exited within a particular time. 
		/// If so it returns success, else it returns failure. It does not redirect the stdout or stderr.
		/// The idea is that the process completes in the given time only if it crashes which is the case causing
		/// error.
		/// </summary>
		/// <param name="filename">
		/// The command to be run.
		/// </param>
		/// <param name="timeout">
		/// The time given for the command to complete.
		/// </param>
		/// <returns>
		/// Returns -1 if the process completes in the given time else returns 0.
		/// </returns>
		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		public static int ExecuteExe(string filename, int timeout)
		{
			int returnCode;
			using (Process proc = new Process())
			{
				proc.EnableRaisingEvents = true;
				proc.StartInfo.Arguments = string.Empty;
				proc.StartInfo.FileName = filename;
				proc.StartInfo.UseShellExecute = false;
				proc.StartInfo.CreateNoWindow = true;
				proc.Start();

				Thread.Sleep(timeout);

				if (proc.HasExited)
				{
					returnCode = -1;
				}
				else
				{
					proc.Kill();
					proc.WaitForExit();
					returnCode = 0;
				}
			}

			return returnCode;
		}

		public static int Execute(string command, TextWriter outStream, TextWriter errStream)
		{
			return Execute(command, outStream, errStream, null, -1);
		}

		public static int Execute(string command, TextWriter outStream, TextWriter errStream, int timeoutInMS)
		{
			return Execute(command, outStream, errStream, null, timeoutInMS);
		}

		public static int Execute(string command, TextWriter outStream, TextWriter errStream, int timeoutInMS, string workingDirectory)
		{
			return Execute(command, outStream, errStream, null, timeoutInMS, workingDirectory);
		}

		public static int Execute(string command, TextWriter outStream, TextWriter errStream, StreamReader inputStream)
		{
			return Execute(command, outStream, errStream, inputStream, -1);
		}

		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		public static int Execute(string command, TextWriter outStream, TextWriter errStream, StreamReader inputStream, int timeoutInMS)
		{
			return Execute(command, outStream, errStream, inputStream, timeoutInMS, null);
		}

		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		public static int Execute(string command, TextWriter outStream, TextWriter errStream, StreamReader inputStream, int timeoutInMS, string workingDirectory)
		{
			return Execute("cmd.exe", String.Concat("/c ", command), outStream, errStream, inputStream, timeoutInMS, workingDirectory);
		}

		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		public static int Execute(string binaryPath, string parameters, TextWriter outStream, TextWriter errStream, StreamReader inputStream, int timeoutInMS, string workingDirectory)
		{
			ScriptExecution se = new ScriptExecution(outStream, errStream);

			int returnCode = -1;

			LogUtils.WriteTrace(DateTime.UtcNow, string.Format("Binary path : {0}", binaryPath));
			LogUtils.WriteTrace(DateTime.UtcNow, string.Format("Binary parameters : {0}", parameters));
			LogUtils.WriteTrace(DateTime.UtcNow, string.Format("Working directory : {0}", workingDirectory));

			using (Process proc = CreateProcess(binaryPath, parameters, outStream, errStream, inputStream, workingDirectory))
			{
				if (-1 != timeoutInMS)
				{
					if (!proc.WaitForExit(timeoutInMS))
					{
						proc.ErrorDataReceived -= se.ErrorDataHandler;
						proc.OutputDataReceived -= se.OutputDataHandler;

						// The ScriptingHelper before throwing TimeoutException exception should
						// disable the output/input handlers of the ScriptExecution class as there
						// can be already scheduled output write events by the Process object in
						// the ThreadPool that will be executed asynchronously after the exception
						// is thrown.  
						// As throwing the exception usually causes the output/input streams in
						// upper layer to be disposed the scheduled output write events by the
						// Process in the ThreadPool that are executed after the disposing would
						// throw in the background thread the ObjectDisposed exception that would cause the process to crash.
						// Therefore disabling here the handlers explicitly before throwing.
						se.DisableDataHandlers = true;
						LogUtils.WriteTrace(DateTime.UtcNow, string.Format(CultureInfo.InvariantCulture, "process {0} {1} timed out!", binaryPath, parameters));
						throw new TimeoutException(string.Format(CultureInfo.InvariantCulture, "process {0} {1} timed out!", binaryPath, parameters));
					}

					// Call to WaitForExit(Int32) returned >>>TRUE<<<, so we've exited without a timeout. But according to documentation, we're not guaranteed to
					// have completed processing of asynchronous events when redirecting stdout. 
					//
					// Workaround is to call WaitForExit(void) a second time which won't return until async events finish processing. Waiting is important
					// so we can safely call StringBuilder.ToString() which is not thread safe, and we don't want event handlers to call StringBuilder.AppendLine 
					// from parallel threads.
					// COMMENTED OUT: This blocks on opened handles if the child process starts another child process and then exists.
					// proc.WaitForExit();
					proc.ErrorDataReceived -= se.ErrorDataHandler;
					proc.OutputDataReceived -= se.OutputDataHandler;
					se.DisableDataHandlers = true;
				}
				else
				{
					proc.WaitForExit();
				}

				returnCode = proc.ExitCode;
			}

			return returnCode;
		}

		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		public static Process CreateProcess(string binaryPath, string parameters, TextWriter outStream, TextWriter errStream, StreamReader inputStream, string workingDirectory)
		{
			ScriptExecution se = new ScriptExecution(outStream, errStream);

			Process proc = new Process();
			proc.EnableRaisingEvents = true;
			proc.ErrorDataReceived += se.ErrorDataHandler;
			proc.OutputDataReceived += se.OutputDataHandler;

			proc.StartInfo.RedirectStandardError = true;
			proc.StartInfo.RedirectStandardOutput = true;
			if (inputStream != null)
			{
				proc.StartInfo.RedirectStandardInput = true;
			}

			proc.StartInfo.Arguments = parameters + string.Empty;
			proc.StartInfo.FileName = binaryPath;
			proc.StartInfo.UseShellExecute = false;
			proc.StartInfo.CreateNoWindow = true;

			if (!string.IsNullOrEmpty(workingDirectory))
			{
				proc.StartInfo.WorkingDirectory = workingDirectory;
			}

			proc.Start();

			proc.BeginErrorReadLine();
			proc.BeginOutputReadLine();

			if (inputStream != null)
			{
				using (StreamWriter destinationStream = proc.StandardInput)
				{
					while (!inputStream.EndOfStream)
					{
						destinationStream.WriteLine(inputStream.ReadLine());
					}
				}
			}

			return proc;
		}
	}
}