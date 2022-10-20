// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Common;

    internal abstract class InfrastructureServiceCommand
    {
        protected const string RestartCommand = "restart";
        protected const string RelocateCommand = "relocate";
        protected const string RemoveCommand = "remove";

        private static readonly TraceType TraceType = new TraceType("Command");

        ////
        // Command Syntax
        //
        // start:<TaskId>:<Node1>:<Task1>:<Node2>:<Task2>...
        // finish:<TaskId>
        ////
        
        private const string StartCommand = "start";
        private const string FinishCommand = "finish";

        private const char CommandDelimiter = ':';

        public abstract string TaskId { get; }

        public abstract long InstanceId { get; }

        public static InfrastructureServiceCommand TryParseCommand(Guid partitionId, string command)
        {
            var temp = command.Split(new char[] { CommandDelimiter }, StringSplitOptions.RemoveEmptyEntries);
            var tokens = new Queue<string>(temp);

            if (tokens.Count <= 0)
            {
                Trace.WriteWarning(TraceType, "Invalid empty command");
                return null;
            }

            var commandType = tokens.Dequeue();

            if (commandType == StartCommand)
            {
                return StartTaskCommand.TryParseCommand(partitionId, tokens);
            }
            else if (commandType == FinishCommand)
            {
                return FinishTaskCommand.TryParseCommand(tokens);
            }
            else
            {
                Trace.WriteWarning(TraceType, "Invalid command type '{0}'", commandType);
                return null;
            }
        }
    }
}