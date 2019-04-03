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
    /// An uploader that consumes queryable etl traces.
    /// </summary>
    internal class AzureTableQueryableEventUploader : AzureTableSelectiveEventUploader
    {
        public AzureTableQueryableEventUploader(ConsumerInitializationParameters initParam)
            : base(initParam)
        {
        }

#if DotNetCoreClrLinux
        protected override bool IsEventInThisTable(EventRecord eventRecord)
        {
            if ((eventRecord.EventHeader.EventDescriptor.Keyword & (ulong)FabricEvents.Keywords.ForQuery) != 0)
            {
                return true;
            }

            return false;
        }
#endif

        protected override bool IsEventConsidered(string providerName, XmlElement eventDefinition)
        {
            string keywordsAttribute = eventDefinition.GetAttribute("keywords");
            string[] keywords = keywordsAttribute.Split(' ');
            foreach (string keyword in keywords)
            {
                if (keyword.Trim().Equals("ForQuery", StringComparison.Ordinal))
                {
                    return true;
                }
            }

            return false;
        }
    }
}
