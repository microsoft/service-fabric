// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Dca;
    using System.Xml;
#if DotNetCoreClrLinux
    using System.Fabric.Common.Tracing;
    using Tools.EtlReader;
#endif

    /// <summary>
    /// An uploader that consumes operational etl traces.
    /// </summary>
    internal class AzureTableOperationalEventUploader : AzureTableSelectiveEventUploader
    {
        public AzureTableOperationalEventUploader(ConsumerInitializationParameters initParam)
            : base(initParam)
        {
        }

#if DotNetCoreClrLinux
        protected override bool IsEventInThisTable(EventRecord eventRecord)
        {
            if ((eventRecord.EventHeader.EventDescriptor.Keyword & TraceEvent.OperationalChannelKeywordMask) != 0)
            {
                return true;
            }

            return false;
        }
#endif

        protected override bool IsEventConsidered(string providerName, XmlElement eventDefinition)
        {
            string channelAttribute = eventDefinition.GetAttribute("channel");
            if (channelAttribute != null && channelAttribute == "Operational")
            {
                return true;
            }

            return false;
        }
    }
}
