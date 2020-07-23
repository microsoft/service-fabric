// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;
    using Linq;
    using Microsoft.Win32;
    using Text.RegularExpressions;
    using System.IO;

    /// <summary>
    /// Helper class for misc. functionality.
    /// </summary>
    internal static class AzureHelper
    {
        /// <summary>
        /// Regular expression used to parse Windows Azure role instance names into the
        /// role name and instance index required to form the Windows Fabric node name.
        /// </summary>
        /// <remarks>
        /// Ideally, parsing the role instance name would be unnecessary.  However, the MR SDK does
        /// not provide the role name and instance index information in separate properties, and relying
        /// on mapping through the ServiceRuntime SDK results in races with the distribution of topology
        /// information.  Parsing manually guarantees that well-formed instance names can always be mapped
        /// to Windows Fabric node names at any time.
        /// </remarks>
        private static readonly Regex RoleInstanceNameRegex = new Regex(
            @"(?<role>.+)_IN_(?<index>\d+)$",
            RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase);

        /// <summary>
        /// Regular expression used to parse Windows Fabric node names into the
        /// role name and instance index required to form the Windows Azure role instance name.
        /// </summary>
        private static readonly Regex NodeNameRegex = new Regex(
            @"(?<role>.+)\.(?<index>\d+)$",
            RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase);

        /// <summary>
        /// Formats a string using <see cref="string.Format(IFormatProvider, string, object[])"/>
        /// where IFormatProvider is <see cref="CultureInfo.InvariantCulture"/>
        /// </summary>
        /// <param name="format">The format to use.</param>
        /// <param name="args">The arguments to be passed in.</param>
        /// <returns>
        /// A copy of format in which the format items have been replaced by the string representations of the corresponding arguments.
        /// </returns>
        /// <remarks>TODO, replace this method with $ style formatting once we move to C# 6.0</remarks>
        public static string ToString(this string format, params object[] args)
        {
            format.Validate("format");
            string text = string.Format(CultureInfo.InvariantCulture, format, args);
            return text;
        }

        /// <summary>
        /// Gets the message of an exception. If the exception is an aggregate exception, then
        /// it combines all the inner exceptions and returns a single string.
        /// </summary>
        /// <param name="ex">The ex.</param>
        /// <returns>The exception message.</returns>
        public static string GetMessage(this Exception ex)
        {
            ex.ThrowIfNull("ex");

            var ae = ex as AggregateException;

            string text = ae != null
                ? string.Join(", ", ae.Flatten().InnerExceptions.Select(e => e.Message))
                : ex.Message;

            return text;
        }

        /// <remarks>
        /// NOTE - In VM scale set (VMSS) world, there is no term called 'role instance' anymore.
        /// </remarks>
        public static string TranslateRoleInstanceToNodeName(this string roleInstanceId)
        {
            roleInstanceId.ThrowIfNullOrWhiteSpace("roleInstanceId");

            Match match = RoleInstanceNameRegex.Match(roleInstanceId);

            if (match.Success)
            {
                string roleName = match.Groups[1].Value;
                int roleInstanceIndex = int.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture);

                return FormatFabricNodeName(roleName, roleInstanceIndex);
            }

            // At this point, it could be in a VMSS format (which we currently don't have documentation for,
            // apart from the fact that it is different from the old-style format which we check above)
            // so, return as-is.
            return roleInstanceId;
        }

        public static string TranslateNodeNameToRoleInstance(this string nodeName)
        {
            nodeName.ThrowIfNullOrWhiteSpace("nodeName");

            Match match = NodeNameRegex.Match(nodeName);

            if (match.Success)
            {
                string roleName = match.Groups[1].Value;
                int roleInstanceIndex = int.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture);

                return FormatRoleInstanceName(roleName, roleInstanceIndex);
            }

            // At this point, it could be in a VMSS format (which we currently don't have documentation for,
            // apart from the fact that it is different from the old-style format which we check above)
            // so, return as-is.
            return nodeName;
        }

        public static string GetTenantId(IConfigSection configSection)
        {
            configSection.Validate("configSection");

            string tenantId = configSection.ReadConfigValue<string>("WindowsAzure.TenantID");
            if (string.IsNullOrWhiteSpace(tenantId))
            {
#if DotNetCoreClrLinux
                tenantId = GetTenantIdOnLinux();
#else
                tenantId = GetTenantIdOnWindows();
#endif
            }

            return tenantId;
        }

        private static string GetTenantIdOnWindows()
        {
            const string TenantIDValueName = "WATenantID";
            string tenantIdKeyName = string.Format(CultureInfo.InvariantCulture, "{0}\\{1}", Registry.LocalMachine.Name, FabricConstants.FabricRegistryKeyPath);

            string tenantId = (string)Registry.GetValue(tenantIdKeyName, TenantIDValueName, null);
            if (tenantId == null)
            {
                // ISSUE: This will no longer be available if Fabric.exe isn't in the WA guest agent's process tree.
                // Will need to have the Windows Fabric startup task write it to a file or to the registry so we can read it here.
                tenantId = Environment.GetEnvironmentVariable("RoleDeploymentID");
            }

            return tenantId;
        }

        private static string GetTenantIdOnLinux()
        {
            // In Linux, the /etc/servicefabric folder serves as the registry specific to SF. 
            // This file serves as the registry key-value equivalent
            // On a Linux VM on public clusters (via SFRP), the Service Fabric VM extension code would have populated this file.
            const string TenantIdFile = @"/etc/servicefabric/WATenantID";

            // any exception (say file doesn't exist etc.), the caller handles it
            string tenantId = File.ReadAllText(TenantIdFile).Trim();
            return tenantId;
        }

        /// <summary>
        /// Determines if the passed in exception is retry-able. This is called by an 
        /// instance <see cref="IRetryPolicy"/>.
        /// </summary>
        /// <param name="e">The exception</param>
        /// <returns><c>true</c> if retry-able; <c>false</c> otherwise.</returns>
        /// <remarks>This is made internal instead of private for unit-test access.</remarks>
        public static bool IsRetriableException(Exception e)
        {
            if (e is FabricTransientException || e is OperationCanceledException || e is TimeoutException)
            {
                return true;
            }

            return false;
        }

        private static string FormatRoleInstanceName(string roleName, int roleIndex)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "{0}_IN_{1}",
                roleName,
                roleIndex);
        }

        private static string FormatFabricNodeName(string roleName, int roleIndex)
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "{0}.{1}",
                roleName,
                roleIndex);
        }
    }
}