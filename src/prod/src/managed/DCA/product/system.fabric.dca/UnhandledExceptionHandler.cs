// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca 
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Dca.Exceptions;
    using System.Fabric.Strings;
    using System.Threading;

    using Health;

    internal static class UnhandledExceptionHandler
    {
        private const string FabricHostSectionName = "FabricHost";
        private const string FabricHostContinuousExitFailureCountResetTimeParameterName = "ContinuousExitFailureCountResetTime";
        private const int FabricHostExitFailureDefaultResetTimeSeconds = 360;
        private const int FabricHostExitFailureResetTimeBufferSeconds = 15;
        private const string TraceType = "UnhandledExceptionHandler";

        private static int fabricHostExitFailureResetTime = FabricHostExitFailureDefaultResetTimeSeconds;

#if !DotNetCoreClr
        // We need to hold onto a reference to the exception so that we can find it in dumps.
        private static Exception lastUnhandledException;
#endif

        internal static ManualResetEvent StopDcaEvent { private get; set; }

        internal static void ReadFabricHostExitFailureResetTime()
        {
            fabricHostExitFailureResetTime = Utility.GetUnencryptedConfigValue(
                                                 FabricHostSectionName,
                                                 FabricHostContinuousExitFailureCountResetTimeParameterName,
                                                 FabricHostExitFailureDefaultResetTimeSeconds);
        }

#if !DotNetCoreClr
        internal static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Exception exception = (Exception)e.ExceptionObject;
            lastUnhandledException = exception;
            LogException(exception);

            try
            {
                if (exception is ConfigurationException)
                {
                    HealthClient.SendNodeHealthReport(
                        string.Format(StringResources.DCAError_ConfigurationUnhandledExceptionHealthDescription, exception.Message),
                        HealthState.Error);
                }
                else
                {
                    HealthClient.SendNodeHealthReport(
                        string.Format(StringResources.DCAError_UnhandledExceptionHealthDescription, exception.Message),
                        HealthState.Error);
                }
            }
            catch (Exception healthClientException)
            {
                Utility.TraceSource.WriteExceptionAsError(TraceType, healthClientException);
            }

            // The DCA is a diagnostic component whose functionality is not critical
            // to the running of the node. Therefore, crashes in the DCA should not
            // cause the node to fail. 
            //
            // However, the current logic in the FabricHost does not make any 
            // distinction between the various Fabric code packages. Therefore the 
            // same failure handling policy is applied to both Fabric.exe and 
            // FabricDCA.exe. The policy is to declare the node as failed if a certain
            // number of consecutive crashes happen within a certain time span. 
            // However, if the process has been running for longer than that time span,
            // then the crash count gets reset.
            //
            // We achieve the goal of not allowing DCA crashes to bring down the node
            // by implementing a workaround for the current FabricHost policy. When an
            // unhandled exception occurs, we check if we have been running longer than
            // the time span at which our failure count gets reset. If not, we sleep
            // in the unhandled exception handler for long enough to ensure that we
            // have been running for longer than the time span.
            DateTime startTime = Process.GetCurrentProcess().StartTime;
            TimeSpan runTimeSoFar = DateTime.Now.Subtract(startTime);
            TimeSpan desiredRunTime = TimeSpan.FromSeconds(fabricHostExitFailureResetTime)
                                      .Add(TimeSpan.FromSeconds(FabricHostExitFailureResetTimeBufferSeconds));
            if (desiredRunTime.CompareTo(runTimeSoFar) > 0)
            {
                TimeSpan sleepTime = desiredRunTime.Subtract(runTimeSoFar);
                if (null == UnhandledExceptionHandler.StopDcaEvent)
                {
                    Thread.Sleep(sleepTime);
                }
                else
                {
                    UnhandledExceptionHandler.StopDcaEvent.WaitOne(sleepTime);
                }
            }
        }

        private static void LogException(Exception e)
        {
            if (null != Utility.TraceSource)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Exception occurred in FabricDCA. Exception information: {0}",
                    e);
            }
        }
#endif
    }
}