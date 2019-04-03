// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// Simple Abstraction to Represent the values from azure table row.
    /// </summary>
    public sealed class AzureTablePropertyBag
    {
        public AzureTablePropertyBag(DateTimeOffset timeLogged, DateTimeOffset timestampInAzureTable, string partitionKey, string rowKey, IDictionary<string, string> properties)
        {
            Assert.AssertNotNull(properties, "properties");
            Assert.AssertIsTrue(properties.Any(), "No properties in map");
            Assert.AssertNotNull(partitionKey, "partitionKey");
            Assert.AssertNotNull(rowKey, "rowKey");
            this.UserDataMap = properties;
            this.TimeLogged = timeLogged;
            this.TimeStamp = timestampInAzureTable;
            this.PartitionKey = partitionKey;
            this.RowKey = rowKey;
        }

        /// <summary>
        /// Partition Key
        /// </summary>
        public string PartitionKey { get; }

        /// <summary>
        /// Row Key
        /// </summary>
        public string RowKey { get; }

        /// <summary>
        /// The time this Row was actually written
        /// </summary>
        public DateTimeOffset TimeLogged { get; }

        /// <summary>
        /// The time it was written in Azure table
        /// </summary>
        public DateTimeOffset TimeStamp { get; }

        /// <summary>
        /// User data Map.
        /// </summary>
        public IDictionary<string, string> UserDataMap { get; }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "TimeLogged: {0}, UserDataMap: {1}",
                this.TimeLogged,
                string.Join(
                    ",",
                    this.UserDataMap.Select(item => string.Format(CultureInfo.InvariantCulture, "PropertyName: {0}, Value: {1}", item.Key, item.Value))));
        }
    }
}