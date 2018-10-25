// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Dca;

#if !DotNetCoreClrLinux
    using Tools.EtlReader;
#endif

    /// <summary>
    /// Factory class to create consumers
    /// </summary>
    internal class ProducerFactory
    {
        // Constants
        private const string TraceType = "ProducerFactory";
        private readonly DiskSpaceManager diskSpaceManager;

        internal ProducerFactory(DiskSpaceManager diskSpaceManager)
        {
            this.diskSpaceManager = diskSpaceManager;
        }
        
        internal IDcaProducer CreateProducer(string producerInstance, ProducerInitializationParameters initParam, string producerType)
        {
            // Create the producer object
            IDcaProducer producer;

#if DotNetCoreClrLinux
            if (producerType.Equals(StandardPluginTypes.LttProducer))
            {
                producer = new LttProducer(
                    this.diskSpaceManager,
                    new TraceEventSourceFactory(),
                    initParam);
            }
#else
            if (producerType.Equals(StandardPluginTypes.EtlFileProducer))
            {
                var configReaderFactory = new EtlProducerConfigReader.EtlProducerConfigReaderFactory(
                        new ConfigReader(initParam.ApplicationInstanceId), 
                        initParam.SectionName);
                producer = new EtlProducer(
                    this.diskSpaceManager,
                    configReaderFactory,
                    new TraceFileEventReaderFactory(),
                    new TraceEventSourceFactory(),
                    initParam);
            }
            else if (producerType.Equals(StandardPluginTypes.EtlInMemoryProducer))
            {
                var configReaderFactory = new EtlInMemoryProducerConfigReader.EtlInMemoryProducerConfigReaderFactory(
                        new ConfigReader(initParam.ApplicationInstanceId),
                        initParam.SectionName);
                producer = new EtlInMemoryProducer(
                    this.diskSpaceManager,
                    configReaderFactory,
                    new TraceFileEventReaderFactory(),
                    new TraceEventSourceFactory(),
                    initParam);
            }
#if !DotNetCoreClrIOT
            else if (producerType.Equals(StandardPluginTypes.PerfCounterConfigReader))
            {
                producer = new PerfCounterConfigReader(initParam);
            }
#endif
#endif
            else
            {
                Debug.Assert(
                    producerType.Equals(StandardPluginTypes.FolderProducer),
                    "No other producer types exist.");
                producer = new FolderProducer(this.diskSpaceManager, initParam);
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "Created producer {0}.",
                producerInstance);

            return producer;
        }
    }
}