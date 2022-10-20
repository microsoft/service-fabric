// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Globalization;
    using Threading.Tasks;

    internal sealed class CommandHandler
    {
        private static readonly char[] CommandTokenDelimiter = { ':' };

        private readonly TraceType traceType;

        public CommandHandler(TraceType traceType)
        {
            this.traceType = traceType.Validate("traceType");

            Commands = new Dictionary<string, CommandHandlerEntry>(StringComparer.OrdinalIgnoreCase);
        }

        public IDictionary<string, CommandHandlerEntry> Commands { get; private set; }

        public async Task<string> ExecuteCommandAsync(string input, bool allowAdminCommands)
        {
            input.Validate("input");

            traceType.WriteInfo("ExecuteCommandAsync: {0}, allow admin commands: {1}", input, allowAdminCommands);

            // Command format is "CommandName:Arguments"
            string[] tokens = input.Split(CommandTokenDelimiter, 2);
            string commandName = tokens[0];

            string args = string.Empty;
            if (tokens.Length > 1)
            {
                args = tokens[1];
            }

            try
            {
                CommandHandlerEntry handlerEntry;
                if (Commands.TryGetValue(commandName, out handlerEntry))
                {
                    if (allowAdminCommands || !handlerEntry.IsAdminCommand)
                    {
                        string result = await handlerEntry.CommandHandlerAsync(args).ConfigureAwait(false);
                        return result;  
                    }

                    string text = string.Format(CultureInfo.InvariantCulture, "Permission denied for command: {0}", commandName);
                    traceType.WriteWarning(text);
                    throw new ArgumentException(text, commandName);
                }

                string text2 = string.Format(CultureInfo.InvariantCulture, "Unrecognized command: {0}", commandName);
                traceType.WriteWarning(text2);
                throw new ArgumentException(text2, commandName);
            }
            catch (Exception e)
            {
                traceType.WriteWarning("Command handler for {0} threw exception: {1}", commandName, e);
                throw;
            }
        }
    }
}