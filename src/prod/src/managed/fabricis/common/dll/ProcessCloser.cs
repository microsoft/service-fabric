// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Threading;

    /// <summary>
    /// This class just for allowing a global way to exit the process.
    /// This exists because CoreCLR doesn't support <see cref="Environment.Exit"/> and we don't
    /// want to use <see cref="Environment.FailFast(string, Exception)"/> in all cases. E.g. the 
    /// uber system is wired up to detect crash dumps and raise an ICM incident that needs to be
    /// investigated and mitigated.
    /// </summary>    
    internal static class ProcessCloser
    {
        // not implementing IDisposable for this class since its job is to just exit the program.
        private static readonly ManualResetEvent processExitEvent = new ManualResetEvent(false);

        /// <summary>
        /// The event that is to be set if FabricIS.exe needs to exit.
        /// </summary>
        public static ManualResetEvent ExitEvent
        {
            get { return processExitEvent; }
        }
    }
}