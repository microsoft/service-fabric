// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ProcessDescription
    {
        internal ProcessDescription()
        {
        }

        internal string ExePath { get; set; }

        internal string Arguments { get; set; }

        internal string StartInDirectory { get; set; }

        internal IDictionary<string, string> EnvVars { get; set; }

        internal string AppDirectory { get; set; }

        internal string TempDirectory { get; set; }

        internal string WorkDirectory { get; set; }

        internal string LogDirectory { get; set; }

        internal bool RedirectConsole { get; set; }

        internal string RedirectedConsoleFileNamePrefix { get; set; }

        internal Int32 ConsoleRedirectionFileRetentionCount { get; set; }

        internal Int32 ConsoleRedirectionFileMaxSizeInKb { get; set; }

        internal bool ShowNoWindow { get; set; }

        internal bool AllowChildProcessDetach { get; set; }

        internal bool NotAttachedToJob { get; set; }

        internal ProcessDebugParameters DebugParameters { get; set; }

        internal ResourceGovernancePolicyDescription ResourceGovernancePolicy { get; set; }

        internal ServicePackageResourceGovernanceDescription ServicePackageResourceGovernance { get; set; }

        internal string CgroupName { get; set; }

        internal bool IsHostedServiceProcess { get; set; }

        internal IDictionary<string, string> EncryptedEnvironmentVariables { get; set; }

        internal static unsafe ProcessDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ProcessDescription.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_PROCESS_DESCRIPTION*)nativePtr);

            IDictionary<string, string> encryptedEnvironmentVariables = new Dictionary<string, string>();

            if (nativeDescription.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_PROCESS_DESCRIPTION_EX1*)nativeDescription.Reserved;
                encryptedEnvironmentVariables = NativeTypes.FromNativeStringPairList(ex1->EncryptedEnvironmentVariables);
            }

            return new ProcessDescription
            {
                ExePath = NativeTypes.FromNativeString(nativeDescription.ExePath),
                Arguments = NativeTypes.FromNativeString(nativeDescription.Arguments),
                StartInDirectory = NativeTypes.FromNativeString(nativeDescription.StartInDirectory),
                EnvVars = NativeTypes.FromNativeStringPairList(nativeDescription.EnvVars),
                AppDirectory = NativeTypes.FromNativeString(nativeDescription.AppDirectory),
                TempDirectory = NativeTypes.FromNativeString(nativeDescription.TempDirectory),
                WorkDirectory = NativeTypes.FromNativeString(nativeDescription.WorkDirectory),
                LogDirectory = NativeTypes.FromNativeString(nativeDescription.LogDirectory),
                RedirectConsole = NativeTypes.FromBOOLEAN(nativeDescription.RedirectConsole),
                RedirectedConsoleFileNamePrefix = NativeTypes.FromNativeString(nativeDescription.RedirectedConsoleFileNamePrefix),
                ConsoleRedirectionFileRetentionCount = nativeDescription.ConsoleRedirectionFileRetentionCount,
                ConsoleRedirectionFileMaxSizeInKb = nativeDescription.ConsoleRedirectionFileMaxSizeInKb,
                ShowNoWindow = NativeTypes.FromBOOLEAN(nativeDescription.ShowNoWindow),
                AllowChildProcessDetach = NativeTypes.FromBOOLEAN(nativeDescription.AllowChildProcessDetach),
                NotAttachedToJob = NativeTypes.FromBOOLEAN(nativeDescription.NotAttachedToJob),
                DebugParameters = ProcessDebugParameters.CreateFromNative(nativeDescription.DebugParameters),
                ResourceGovernancePolicy = ResourceGovernancePolicyDescription.CreateFromNative(nativeDescription.ResourceGovernancePolicy),
                ServicePackageResourceGovernance = ServicePackageResourceGovernanceDescription.CreateFromNative(nativeDescription.ServicePackageResourceGovernance),
                CgroupName = NativeTypes.FromNativeString(nativeDescription.CgroupName),
                IsHostedServiceProcess = NativeTypes.FromBOOLEAN(nativeDescription.IsHostedServiceProcess),
                EncryptedEnvironmentVariables = encryptedEnvironmentVariables
            };
        }
    }
}