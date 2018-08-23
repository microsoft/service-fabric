// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.IO;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Fabric.Management.ServiceModel;
    using System.Xml;
    using System.Xml.Serialization;

    using Microsoft.Win32;
    using System.Fabric.Common;

    internal class InvalidCommandLineParametersException : Exception
    {
        public InvalidCommandLineParametersException(string message)
            : base(message)
        {
        }
    }

    internal class CommandLineInfo
    {
        private const string OperationString = "/operation";

        private CommandLineInfo()
        {
        }

        public static DeploymentParameters Parse(string[] args)
        {
            DeploymentParameters parameters = new DeploymentParameters();
            if (args.Length == 0)
            {
                parameters.CreateFromFile();
                return parameters;
            }

            Dictionary<string, dynamic> commandArgs = new Dictionary<string, dynamic>(StringComparer.OrdinalIgnoreCase);
            string operationString = null;
            foreach (string arg in args)
            {
                int delimiterIndex = arg.IndexOf(":", StringComparison.Ordinal);

                if (delimiterIndex == -1)
                {
                    PrintUsage(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ArgumentInvalid_Formatted_arg1, arg));
                }

                string key = arg.Substring(0, delimiterIndex);
                string value = arg.Substring(delimiterIndex + 1);
                if (CommandLineInfo.OperationString.Equals(key, StringComparison.OrdinalIgnoreCase))
                {
                    operationString = value;
                    continue;
                }

                if (!parameters.IsSupportedParameter(key))
                {
                    PrintUsage(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ArgumentUnexpected_Formatted, key));
                }

                if (commandArgs.ContainsKey(key))
                {
                    PrintUsage(string.Format(CultureInfo.CurrentCulture, StringResources.Error_CommandLineArgumentFoundMoreThanOnce_Formatted, key));
                }

                commandArgs.Add(key, value);
            }

            var operation = GetOperation(operationString);
            parameters.SetParameters(commandArgs, operation);
            return parameters;
        }

        private static DeploymentOperations GetOperation(string operation)
        {
            if (string.IsNullOrEmpty(operation))
            {
                PrintUsage(string.Format(CultureInfo.CurrentCulture, StringResources.Error_RequiredCmdlineArgumentNotFound_Formatted, OperationString));
            }

            DeploymentOperations parsedOperationValue;
            if (!Enum.TryParse<DeploymentOperations>(operation, true, out parsedOperationValue))
            {
                PrintUsage(string.Format(CultureInfo.CurrentCulture, StringResources.Error_OperationValueNotSupported_Formatted, operation));
            }

            return parsedOperationValue;
        }

        private static void PrintUsage(string errorMessage)
        {
            Console.WriteLine(StringResources.General_Usage);
            throw new InvalidCommandLineParametersException(errorMessage);
        }
    }
}