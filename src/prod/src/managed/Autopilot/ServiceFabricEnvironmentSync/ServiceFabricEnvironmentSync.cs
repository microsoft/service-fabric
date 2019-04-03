// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{    
    using System;
    using System.IO;
    using System.Reflection;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.Search.Autopilot;

    internal class ServiceFabricEnvironmentSync
    {
        private readonly Action<bool> exitCallback;

        private EnvironmentCertificateManager certificateManager;

        private Task certificateManagerTask;

        private CancellationTokenSource certificateManagerCancellationTokenSource;

        public ServiceFabricEnvironmentSync(Action<bool> exitCallback)
        {
            this.exitCallback = exitCallback;

            this.certificateManager = null;

            this.certificateManagerTask = null;

            this.certificateManagerCancellationTokenSource = new CancellationTokenSource();
        }

        public void Start()
        {
            string applicationDirectory = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);

            this.certificateManager = new EnvironmentCertificateManager(applicationDirectory);

            APRuntime.Initialize(StringConstants.ApplicationConfigurationFileName);

            TextLogger.InitializeLogging(StringConstants.CustomLogIdName);

            TextLogger.LogInfo("Starting Service Fabric Environment Sync.");

            if (!ConfigurationManager.GetCurrentConfigurations())
            {
                this.Exit(false);

                return;
            }

            TextLogger.SetLogLevel(ConfigurationManager.LogLevel);            

            if (!this.certificateManager.GetEnvironmentCertificatesFromCurrentConfigurations())
            {
                this.Exit(false);

                return;
            }            

            this.RegisterConsoleCancelEventNotifications();

            this.certificateManagerTask = Task.Factory.StartNew(
                                                () => this.RunCertificateManager(this.certificateManagerCancellationTokenSource.Token),
                                                this.certificateManagerCancellationTokenSource.Token,
                                                TaskCreationOptions.LongRunning,
                                                TaskScheduler.Default);
        }
 
        private async Task RunCertificateManager(CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                bool succeeded = this.certificateManager.EnsureEnvironmentCertificatesPresence();

                if (succeeded)
                {
                    await Task.Delay(TimeSpan.FromSeconds(ConfigurationManager.CertificateManagerRunIntervalSeconds), cancellationToken);
                }
                else
                {
                    TextLogger.LogInfo("Certificate manager run failed. Retry after {0} seconds.", ConfigurationManager.CertificateManagerRetryIntervalSeconds);

                    await Task.Delay(TimeSpan.FromSeconds(ConfigurationManager.CertificateManagerRetryIntervalSeconds), cancellationToken);
                }
            }
        }

        private void ConsoleCancelEventHanlder(object sender, ConsoleCancelEventArgs args)
        {
            TextLogger.LogInfo("Cancel event (Ctrl + C / Ctrl + Break) received. Stopping Service Fabric Environment Sync.");

            this.certificateManagerCancellationTokenSource.Cancel();

            TextLogger.LogInfo("Service Fabric Environment Sync has stopped. Exiting the process.");

            this.Exit(true);
        }

        private void RegisterConsoleCancelEventNotifications()
        {
            Console.CancelKeyPress += new ConsoleCancelEventHandler(ConsoleCancelEventHanlder);
        }

        private void Exit(bool succeeded)
        {
            TextLogger.Flush();

            this.exitCallback(succeeded);
        }
    }
}