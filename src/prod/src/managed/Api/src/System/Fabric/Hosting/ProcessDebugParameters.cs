// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ProcessDebugParameters
    {
        internal ProcessDebugParameters()
        {
        }

        internal string ExePath { get; set; }

        internal string Arguments { get; set; }

        internal string LockFile { get; set; }

        internal string WorkingFolder { get; set; }

        internal string DebugParametersFile { get; set; }

        internal IDictionary<string, string> EnvVars { get; set; }

        internal List<string> ContainerEntryPoints { get; set; }

        internal List<string> ContainerMountedVolumes { get; set; }

        internal List<string> ContainerEnvironmentBlock { get; set; }

        internal List<string> ContainerLabels { get; set; }

        internal static unsafe ProcessDebugParameters CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ProcessDebugParameters.CreateFromNative() has null pointer.");

            var nativeParameters = *((NativeTypes.FABRIC_PROCESS_DEBUG_PARAMETERS*)nativePtr);

            var debugParams = new ProcessDebugParameters
            {
                ExePath = NativeTypes.FromNativeString(nativeParameters.ExePath),
                Arguments = NativeTypes.FromNativeString(nativeParameters.Arguments),
                LockFile = NativeTypes.FromNativeString(nativeParameters.LockFile),
                WorkingFolder = NativeTypes.FromNativeString(nativeParameters.WorkingFolder),
                DebugParametersFile = NativeTypes.FromNativeString(nativeParameters.DebugParametersFile),
                EnvVars = NativeTypes.FromNativeStringPairList(nativeParameters.EnvVars),
                ContainerEntryPoints = NativeTypes.FromNativeStringList(nativeParameters.ContainerEntryPoints),
                ContainerMountedVolumes = NativeTypes.FromNativeStringList(nativeParameters.ContainerMountedVolumes),
                ContainerEnvironmentBlock = NativeTypes.FromNativeStringList(nativeParameters.ContainerEnvironmentBlock)
            };

            if(nativeParameters.Reserved != null)
            {
                var nativeParametersx1 = *((NativeTypes.FABRIC_PROCESS_DEBUG_PARAMETERS_EX1*)nativeParameters.Reserved);

                debugParams.ContainerLabels = NativeTypes.FromNativeStringList(nativeParametersx1.ContainerLabels);
            }

            return debugParams;
        }
    }
}