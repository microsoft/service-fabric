// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Health;
    using System.Fabric.Strings;
    using System.IO;
    using System.Net;
    using System.Threading;
    using Microsoft.Win32;

    // This class implements the general start and stop logic for the DCA
    internal class Program
    {
        // Constants
        private const string TraceType = "Program";

        // This event is set when the DCA needs to stop
        private static ManualResetEvent stopDCAEvent;

        // This event is set when the DCA has taken the necessary actions to stop
        private static ManualResetEvent dcaHasStoppedEvent;

        public static int Main(string[] args)
        {
            Utility.TraceSource = null;

#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
            // Register an unhandled exception event handler
            AppDomain.CurrentDomain.UnhandledException += UnhandledExceptionHandler.OnUnhandledException;
#endif

            // Initialize config store
            ConfigUpdateHandler configUpdateHandler = new ConfigUpdateHandler();
            Utility.InitializeConfigStore(configUpdateHandler);

            // Initialize the Fabric node Id
            Utility.InitializeFabricNodeInfo();

            // Read FabricHost's exit failure reset time from settings
            UnhandledExceptionHandler.ReadFabricHostExitFailureResetTime();

            // Set up tracing
            Utility.InitializeTracing();
            Utility.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

            Utility.DcaProgramDirectory = Path.Combine(Path.GetDirectoryName(FabricEnvironment.GetCodePath()), "DCA.Code");

            // Initialize the work directory
            Utility.InitializeWorkDirectory();

            // Create the event that indicates when the main thread should
            // stop the DCA
            stopDCAEvent = new ManualResetEvent(false);
            UnhandledExceptionHandler.StopDcaEvent = stopDCAEvent;

            // Create the event that indicates that the main thread has 
            // stopped the DCA.
            dcaHasStoppedEvent = new ManualResetEvent(false);

            // Register to be notified when we need to stop.
            Console.CancelKeyPress += CtrlCHandler;

            // The configuration update handler makes use of some members that we just
            // initialized above, e.g. the trace source and the event used to indicate
            // that the DCA should stop. 
            //
            // Until we initialize these members, it is not safe for the the configuration
            // update handler to process any updates because it would end up trying to 
            // access those uninitialized members. Therefore the configuration update 
            // handler ignores these updates until this point. This is okay, because we
            // haven't attempted to read configurations until now. When we read the 
            // configurations later, we will be reading the updated values anyway, so 
            // ignoring the update notifications is not a problem.
            // 
            // Now that we have initialized all the members needed by the configuration
            // update handler, we tell it to start processing configuration updates.
            configUpdateHandler.StartProcessingConfigUpdates();

            // Create and initialize the application instance manager object.
            AppInstanceManager appInstanceMgr = new AppInstanceManager();

            // Notify the application instance manager about the availability of the
            // Windows Fabric application.
            appInstanceMgr.CreateApplicationInstance(Utility.WindowsFabricApplicationInstanceId, null);

            // Let the config update handler know that the Windows Fabric
            // Application has been created, so that it can send configuration
            // updates if it needs to.
            configUpdateHandler.StartConfigUpdateDeliveryToAppInstanceMgr(appInstanceMgr);

            // Create and initialize the service package table manager object
            ServicePackageTableManager servicePackageTableManager = new ServicePackageTableManager(appInstanceMgr);

            // Create and initialize the container manager object if required.
            ContainerManager containerManager = null;
            ContainerEnvironment containerEnvironment = null;

            // FabricContainerAppsEnabled is set to true by default
            bool enableContainerManager = Utility.GetUnencryptedConfigValue<bool>(
                ConfigReader.HostingSectionName,
                ConfigReader.FabricContainerAppsEnabledParameterName,
                true);

            if (enableContainerManager)
            {
                containerEnvironment = new ContainerEnvironment();
                containerManager = new ContainerManager(appInstanceMgr, containerEnvironment);
            }

            // DCA is running again.
            HealthClient.ClearNodeHealthReport();

            // Wait for the event that is signaled when the DCA needs to be
            // stopped.
            stopDCAEvent.WaitOne();

            // Stop the DCA's periodic activities
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Stopping the DCA ...");

            servicePackageTableManager.Dispose();
            configUpdateHandler.Dispose();
            appInstanceMgr.Dispose();

            Utility.TraceSource.WriteInfo(
                TraceType,
                "DCA has stopped.");

            // Set event to indicate that the main thread has stopped the DCA
            bool result = dcaHasStoppedEvent.Set();
            System.Fabric.Interop.Utility.ReleaseAssert(
                result,
                StringResources.DCAError_SignalEventToStopFailed);

            return 0;
        }

        private static void StopDCA()
        {
            // Stop the DCA asynchronously
            StopDCAAsync();

            // Wait on the event that indicates that the main thread has stopped
            // the DCA.
            //
            // FabricHost enforces a timeout for stop, so we don't use a timeout
            // here while waiting.
            dcaHasStoppedEvent.WaitOne();
        }

        private static void StopDCAAsync()
        {
            // Tell the main thread to stop the DCA
            bool result = stopDCAEvent.Set();
            System.Fabric.Interop.Utility.ReleaseAssert(
                result,
                StringResources.DCAError_SignalEventToStopFailed_2);
        }

        private static void CtrlCHandler(object sender, ConsoleCancelEventArgs eventArgs)
        {
            Utility.TraceSource.WriteInfo(
                TraceType,
                "Stopping DCA due to Ctrl+C ...");

            StopDCA();

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Exiting Ctrl+C handler ...");
        }
    }
}