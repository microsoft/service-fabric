// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Runtime.Serialization;

    [DataContract]
    public class PropertyBagValueReader : ValueReader
    {
        // Doesn't this needs to be serialized? TODO
        private IDictionary<string, string> nameValueMap;

        /// <summary>
        /// An Array of metadata about the property value we can read
        /// </summary>
        [DataMember]
        private string[] propertyNameIndexed;

        [DataMember]
        private Dictionary<int, object> missingIndexesAndDefaultValueMap;

        public PropertyBagValueReader(PropertyMetadataData[] propertyMetadataList, int version, IDictionary<string, string> propertyValues)
        {
            Assert.AssertNotNull(propertyMetadataList, "propertyMetadataList");
            Assert.AssertNotNull(propertyValues, "propertyValues");
            Assert.AssertIsTrue(propertyValues.Count >= propertyMetadataList.Length, "Number of Values < Number of Properties");
            this.nameValueMap = propertyValues;

            this.propertyNameIndexed = new string[propertyMetadataList.Length];

            this.missingIndexesAndDefaultValueMap = new Dictionary<int, object>();

            // TODO: The creation of propertyNameIdex can be cached in the creator since it will done only 1 Time for each Type during a Run.
            for (int i = 0; i < propertyMetadataList.Length; ++i)
            {
                var onePropertyMetadataData = propertyMetadataList[i];

                var attribute = onePropertyMetadataData.AttributeData;
                if (attribute.Version != null && attribute.Version.HasValue && attribute.Version.Value > version)
                {
                    this.missingIndexesAndDefaultValueMap.Add(i, attribute.DefaultValue);
                    continue;
                }

                // Note: This is not ideal but required for Azure Table scenario. Today, while uploading, if any property Name
                // contains '.', it's replaced by '_'. Now when we are reading it back, we have to handle that scenario. We can't
                // do a blanket '_' to '.' transformation on the data read since some property may have '_' to being with. Hence
                // this check at this place.
                var transformedName = onePropertyMetadataData.AttributeData.OriginalName.Replace('.', '_');

                this.propertyNameIndexed[onePropertyMetadataData.AttributeData.Index] = transformedName;
                if (!this.nameValueMap.ContainsKey(transformedName))
                {
                    throw new ArgumentException(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Property: {0} Missing. Property Map: {1}",
                            onePropertyMetadataData.AttributeData.OriginalName,
                            string.Join(
                                ",",
                                propertyValues.Select(item => string.Format(CultureInfo.InvariantCulture, "Key: {0}, Value: {1}", item.Key, item.Value)))));
                }
            }
        }

        public override string ReadUtf8StringAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (string)this.missingIndexesAndDefaultValueMap[index];
            }

            return this.nameValueMap[this.propertyNameIndexed[index]];
        }

        public override IPAddress ReadIpAddrV6At(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (IPAddress)this.missingIndexesAndDefaultValueMap[index];
            }

            return IPAddress.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override Guid ReadGuidAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (Guid)this.missingIndexesAndDefaultValueMap[index];
            }

            return Guid.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override DateTime ReadDateTimeAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (DateTime)this.missingIndexesAndDefaultValueMap[index];
            }

            return DateTime.SpecifyKind(DateTime.Parse(this.nameValueMap[this.propertyNameIndexed[index]]), DateTimeKind.Utc);
        }

        public override string ReadUnicodeStringAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (string)this.missingIndexesAndDefaultValueMap[index];
            }

            return this.nameValueMap[this.propertyNameIndexed[index]];
        }

        public override byte[] ReadByteArrayAt(int index, int size)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (byte[])this.missingIndexesAndDefaultValueMap[index];
            }

            throw new NotImplementedException();
        }

        public override bool ReadBoolAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (bool)this.missingIndexesAndDefaultValueMap[index];
            }

            return bool.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override byte ReadByteAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (byte)this.missingIndexesAndDefaultValueMap[index];
            }

            return byte.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override short ReadInt16At(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (short)this.missingIndexesAndDefaultValueMap[index];
            }

            return short.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override int ReadInt32At(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (int)this.missingIndexesAndDefaultValueMap[index];
            }

            return int.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override long ReadInt64At(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (long)this.missingIndexesAndDefaultValueMap[index];
            }

            return long.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override float ReadSingleAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (float)this.missingIndexesAndDefaultValueMap[index];
            }

            return float.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override double ReadDoubleAt(int index)
        {
            if (this.missingIndexesAndDefaultValueMap.ContainsKey(index))
            {
                return (double)this.missingIndexesAndDefaultValueMap[index];
            }

            return double.Parse(this.nameValueMap[this.propertyNameIndexed[index]]);
        }

        public override ValueReader Clone()
        {
            // TODO: Should we be doing deep copies here? Think in details.
            var clonedReader = (PropertyBagValueReader)this.MemberwiseClone();
            return clonedReader;
        }
    }
}