// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Globalization;
    using Threading.Tasks;

    internal sealed class CommandHandlerEntry
    {
        public CommandHandlerEntry(
            string command,             
            Func<string, Task<string>> commandHandlerAsync,
            string description,
            bool isAdminCommand = false,
            bool documented = true)
        {
            this.Command = command.Validate("command");
            this.Description = description.Validate("description");
            this.CommandHandlerAsync = commandHandlerAsync.Validate("commandHandlerAsync");
            this.IsAdminCommand = isAdminCommand;
            this.Documented = documented;
        }

        public string Command { get; private set; }

        public string Description { get; private set; }

        public bool IsAdminCommand { get; private set; }

        /// <summary>
        /// Is this command for internal debugging or is it documented for public use.
        /// </summary>
        public bool Documented { get; private set; }

        public Func<string, Task<string>> CommandHandlerAsync { get; private set; }

        public override string ToString()
        {
            string text = string.Format(CultureInfo.InvariantCulture, "{0}, is admin: {1}, documented: {2}", Command, IsAdminCommand, Documented);
            return text;
        }
    }
}