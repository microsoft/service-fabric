// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.ClientSwitch
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Testability.Client;
    using System.Fabric.Testability.Common;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Text;
    using System.Threading.Tasks;

    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    public class PowerShellManager
    {
        private const string TraceSource = "PowershellManager";
        private readonly RunspaceGroup runspaceGroup;
        private Action<Runspace> initialAction;

        public PowerShellManager()
            : this(DefaultInitialAction)
        {
        }

        public PowerShellManager(Action<Runspace> initialAction)
        {
            this.initialAction = initialAction;
            this.runspaceGroup = RunspaceGroup.Instance;
        }

        public async Task<uint> ExecuteAndConvertExceptionToUintAsync(Func<Task> operation)
        {
            NativeTypes.FABRIC_ERROR_CODE errorCode = await HandleExceptionAsync(operation);
            return (uint)errorCode;
        }

        internal async Task<NativeTypes.FABRIC_ERROR_CODE> HandleExceptionAsync(Func<Task> operation)
        {
            // TODO: this currently doesn't catch all exceptions
            // For example, if there is a null reference exception, it will return error code success
            // This should probably be changed to catch all
            NativeTypes.FABRIC_ERROR_CODE errorCode = 0;

            try
            {
                await operation();
            }
            catch (CmdletInvocationException e)
            {
                if (!FabricClientState.TryGetErrorCode(e.InnerException, out errorCode))
                {
                    throw;
                }
            }
            catch (AggregateException e)
            {
                bool foundErrorCode = false;
                if (e.InnerExceptions != null)
                {
                    foreach (Exception inner in e.Flatten().InnerExceptions)
                    {
                        if (FabricClientState.TryGetErrorCode(inner, out errorCode))
                        {
                            foundErrorCode = true;
                            break;
                        }
                    }
                }

                if (!foundErrorCode)
                {
                    throw;
                }
            }

            return errorCode;
        }

        private static void LogFailure(Command command, Exception ex)
        {
            TestabilityTrace.TraceSource.WriteWarning(TraceSource, "Command: {0} {1}; Error Message: {2}", command.CommandText, CreateParamsString(command), ex.ToString());
        }

        public Task<PSDataCollection<PSObject>> ExecutePowerShellCommandAsync(string commandName, CommandParameterCollection parameters, string script)
        {
            ThrowIf.NullOrEmpty(commandName, "commandName");
            ThrowIf.Null(parameters, "parameters");

            return Task.Factory.StartNew(() => this.ExecutePowerShellCommand(commandName, parameters, script));
        }

        public Task<PSDataCollection<PSObject>> ExecutePowerShellCommandAsync(string commandName, CommandParameterCollection parameters)
        {
            return this.ExecutePowerShellCommandAsync(commandName, parameters, string.Empty);
        }

        public PSDataCollection<PSObject> ExecutePowerShellCommand(string commandName, CommandParameterCollection parameters, string script)
        {
            Command command = new Command(commandName);
            foreach (CommandParameter parameter in parameters)
            {
                command.Parameters.Add(parameter);
            }

            return this.ExecutePowerShellCommand(command, script);
        }


        public PSDataCollection<PSObject> ExecutePowerShellCommand(Command command, string script)
        {
            try
            {
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceSource, 
                    "Executing powershell command \"{0}\" with parameters listed below", 
                    command.ToString());

                foreach (var param in command.Parameters)
                {
                    TestabilityTrace.TraceSource.WriteInfo(
                    TraceSource,
                    "Parameter: {0} Value {1}",
                    param.Name,
                    param.Value);
                }

                var timer = new System.Diagnostics.Stopwatch();
                timer.Start();
                using (RunspaceGroup.Token runspaceToken = this.runspaceGroup.Take())
                using (PowerShell shell = PowerShell.Create())
                {
                    timer.Stop();
                    if(timer.Elapsed > TimeSpan.FromSeconds(1))
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceSource, "ExecutePowerShellCommand took {0} to start: \"{1}\"", timer.Elapsed.ToString(), command.ToString());
                    }

                    // Initial action to be executed before any operation. In this case it should be a connect cluster
                    this.initialAction(runspaceToken.Runspace);

                    shell.Runspace = runspaceToken.Runspace;
                    if(!string.IsNullOrEmpty(script))
                    {
                        shell.Commands.AddScript(script);
                    }

                    shell.Commands.AddCommand(command);

                    IEnumerable<PSObject> results = shell.Invoke();
                    LogResults(command, results);
                    ThrowIfError(shell);

                    return Disposable.Guard(() => new PSDataCollection<PSObject>(results), (d) => { });
                }
            }
            catch (Exception ex)
            {
                LogFailure(command, ex);
                throw;
            }
        }

        private static void ThrowIfError(PowerShell shell)
        {
            ThrowIf.Null(shell, "shell");

            if (shell.Streams.Error.Count > 0)
            {
                List<Exception> exceptions = new List<Exception>();
                foreach (ErrorRecord errorRecord in shell.Streams.Error)
                {
                    Exception innerException = errorRecord.Exception;
                    if (errorRecord.CategoryInfo != null)
                    {
                        innerException.Data.Add("Category", errorRecord.CategoryInfo.Category);
                    }

                    exceptions.Add(innerException);
                }

                throw new AggregateException("PowerShell exceptions occurred", exceptions);
            }
        }

        private static string CreateParamsString(Command command)
        {
            StringBuilder paramsBuilder = new StringBuilder();
            if (command.Parameters != null)
            {
                foreach (CommandParameter parameter in command.Parameters)
                {
                    paramsBuilder.Append('-');
                    paramsBuilder.Append(parameter.Name);
                    paramsBuilder.Append(' ');

                    if (parameter.Value != null)
                    {
                        paramsBuilder.Append(parameter.Value);
                        paramsBuilder.Append(' ');
                    }
                }
            }

            return paramsBuilder.ToString();
        }

        private static void DefaultInitialAction(Runspace runspace)
        {
        }

        private static void LogResults(Command command, IEnumerable<PSObject> results)
        {
            if (results == null)
            {
                TestabilityTrace.TraceSource.WriteNoise(TraceSource, "PowerShellClient Results: <null>");
                return;
            }

            StringBuilder outputBuilder = new StringBuilder();
            foreach (PSObject obj in results)
            {
                if (obj == null)
                {
                    TestabilityTrace.TraceSource.WriteNoise(TraceSource, "PowerShellClient Result: <null>");
                    continue;
                }

                outputBuilder.Append(obj);
                outputBuilder.AppendLine();
                foreach (PSPropertyInfo property in obj.Properties)
                {
                    outputBuilder.Append(property.Name);
                    outputBuilder.Append(':');
                    outputBuilder.Append(property.Value == null ? "null" : property.Value.ToString());
                    outputBuilder.AppendLine();
                }
            }

            TestabilityTrace.TraceSource.WriteNoise(TraceSource, "Command: {0} {1}; Results: {2}", command.CommandText, CreateParamsString(command), outputBuilder.ToString());
        }
    }
}


#pragma warning restore 1591