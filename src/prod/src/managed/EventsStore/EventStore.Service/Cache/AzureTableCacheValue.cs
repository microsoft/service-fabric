// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;

    internal class AzureTableCacheValue : ICacheValue
    {
        private AzureTablePropertyBag azurePropertyBag;

        public AzureTableCacheValue(AzureTablePropertyBag propBag)
        {
            Assert.IsNotNull(propBag, "progBag");
            this.azurePropertyBag = propBag;
        }

        public AzureTablePropertyBag AzureTablePropertyBag
        {
            get { return this.azurePropertyBag; }
        }

        public IDictionary<string, string> DataMap
        {
            get { return this.azurePropertyBag.UserDataMap; }
        }

        public DateTimeOffset ValueCreateTime
        {
            get
            {
                return this.azurePropertyBag.TimeLogged;
            }
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
            {
                throw new ArgumentNullException("obj");
            }

            var other = obj as AzureTableCacheValue;
            if (other == null)
            {
                throw new ArgumentException("obj");
            }

            return this.azurePropertyBag.TimeLogged.CompareTo(other.azurePropertyBag.TimeLogged);
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "PartitionKey: {0}, RowKey: {1}, TimeLogged: {2}, Properties: {3}",
                this.azurePropertyBag.PartitionKey,
                this.azurePropertyBag.RowKey,
                this.azurePropertyBag.TimeLogged,
                string.Join(";", this.azurePropertyBag.UserDataMap.Select(item => string.Format("{0} = {1}", item.Key, item.Value))));
        }
    }
}