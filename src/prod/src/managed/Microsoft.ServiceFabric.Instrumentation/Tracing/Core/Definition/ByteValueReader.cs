// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Text;

    [DataContract]
    [KnownType(typeof(PropertyMetadataWrapper))]
    public unsafe class ByteValueReader : ValueReader
    {
        /// <summary>
        /// Version of the Event whose values are being read.
        /// </summary>
        [DataMember] private int version;

        /// <summary>
        /// Data blob
        /// </summary>
        /// <remarks>
        /// The reason we keep a byte array, instead of just working with the unmanaged memory pointer we received is our need to serialize and de-serialize.
        /// </remarks>
        [DataMember] private byte[] userData;

        /// <summary>
        /// An Array of metadata about the property value we can read
        /// </summary>
        [DataMember] private PropertyMetadataWrapper[] propertyMetadataWrapperList;

        /// <summary>
        /// Create an instance of <see cref="ByteValueReader"/>
        /// </summary>
        /// <param name="propertyMetadataList"></param>
        /// <param name="rawTraceData"></param>
        /// <param name="rawDataLength"></param>
        public ByteValueReader(PropertyMetadataData[] propertyMetadataList, int version, IntPtr rawTraceData, ushort rawDataLength)
        {
            Assert.AssertNotNull(propertyMetadataList, "propertyMetadataList");
            Assert.AssertIsFalse(rawTraceData == IntPtr.Zero, "Raw Data is Null");

            this.version = version;

            // Sort the metadatalist by index.
            this.propertyMetadataWrapperList = propertyMetadataList.OrderBy(item => item.AttributeData.Index).Select(item => new PropertyMetadataWrapper(item))
                .ToArray();

            this.userData = new byte[rawDataLength];
            Marshal.Copy(rawTraceData, this.userData, 0, rawDataLength);
        }

        /// <inheritdoc />
        public override string ReadUtf8StringAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<string>(index);
            }

            if (this.GetOffsetForPropertyAtIndex(index) >= this.userData.Length)
            {
                throw new Exception(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Offset: {0} is beyond Data Length: {1}",
                        this.GetOffsetForPropertyAtIndex(index),
                        this.userData.Length));
            }

            fixed (byte* pointer = this.userData)
            {
                var buff = new byte[this.userData.Length];
                byte* ptr = pointer + this.GetOffsetForPropertyAtIndex(index);
                int i = 0;
                while (i < buff.Length)
                {
                    byte c = ptr[i];
                    if (c == 0)
                        break;
                    buff[i++] = c;
                }
                return Encoding.UTF8.GetString(buff, 0, i); // Convert to unicode.  
            }
        }

        /// <inheritdoc />
        public override IPAddress ReadIpAddrV6At(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<IPAddress>(index);
            }

            byte[] addrBytes = new byte[16];
            for (int i = 0; i < addrBytes.Length; i++)
                addrBytes[i] = this.ReadByteAtOffset(this.GetOffsetForPropertyAtIndex(index) + i);
            return new IPAddress(addrBytes);
        }

        /// <inheritdoc />
        public override Guid ReadGuidAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<Guid>(index);
            }

            fixed (byte* pointer = this.userData)
            {
                return *((Guid*)(pointer + this.GetOffsetForPropertyAtIndex(index)));
            }
        }

        /// <inheritdoc />
        /// <remarks>
        /// We always assume that time read is in UTC timezone and mark the returned object as UTC.
        /// </remarks>
        public override DateTime ReadDateTimeAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<DateTime>(index);
            }

            var time = DateTime.FromFileTime(this.ReadInt64AtOffset(this.GetOffsetForPropertyAtIndex(index)));
            return DateTime.SpecifyKind(time, DateTimeKind.Utc);
        }

        /// <inheritdoc />
        public override string ReadUnicodeStringAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<string>(index);
            }

            if (this.GetOffsetForPropertyAtIndex(index) >= this.userData.Length)
                throw new Exception("Reading past end of event");

            // To avoid scanning the string twice we first check if the last character
            // in the buffer is a 0 if so, we KNOW we can use the 'fast path'  Otherwise we check 

            var offset = this.GetOffsetForPropertyAtIndex(index);
            fixed (byte* ptr = this.userData)
            {
                char* charEnd = (char*)(ptr + this.userData.Length);
                if (charEnd[-1] == 0) 
                    return new string((char*)(ptr + offset)); // We can count on a null, so we do an optimized path 

                // unoptimized path.  Carefully count characters and create a string up to the null.  
                char* charStart = (char*)(ptr + offset);
                int maxPos = (this.userData.Length - offset) / sizeof(char);
                int curPos = 0;
                while (curPos < maxPos && charStart[curPos] != 0)
                    curPos++;

                // CurPos now points at the end (either buffer end or null terminator, make just the right sized string.  
                return new string(charStart, 0, curPos);
            }
        }

        /// <inheritdoc />
        public override byte[] ReadByteArrayAt(int index, int size)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<byte[]>(index);
            }

            byte[] res = new byte[size];
            for (int i = 0; i < size; i++)
            {
                res[i] = this.ReadByteAtOffset(this.GetOffsetForPropertyAtIndex(index) + i);
            }
            return res;
        }

        /// <inheritdoc />
        public override bool ReadBoolAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<bool>(index);
            }

            // For byte serialization, we read a byte for boolean.
            return this.ReadByteAt(index) != 0;
        }

        /// <inheritdoc />
        public override byte ReadByteAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<byte>(index);
            }

            return this.ReadByteAtOffset(this.GetOffsetForPropertyAtIndex(index));
        }

        /// <inheritdoc />
        public override short ReadInt16At(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<short>(index);
            }

            fixed (byte* pointer = this.userData)
            {
                return *((short*)(pointer + this.GetOffsetForPropertyAtIndex(index)));
            }
        }

        /// <inheritdoc />
        public override int ReadInt32At(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<int>(index);
            }

            fixed (byte* pointer = this.userData)
            {
                return *((int*)(pointer + this.GetOffsetForPropertyAtIndex(index)));
            }
        }

        /// <inheritdoc />
        public override long ReadInt64At(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<long>(index);
            }

            return ReadInt64AtOffset(this.GetOffsetForPropertyAtIndex(index));
        }

        /// <inheritdoc />
        public override float ReadSingleAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<float>(index);
            }

            fixed (byte* pointer = this.userData)
            {
                return *((float*)(pointer + this.GetOffsetForPropertyAtIndex(index)));
            }
        }

        /// <inheritdoc />
        public override double ReadDoubleAt(int index)
        {
            if (!this.IsIndexAvailableInCurrentVersion(index))
            {
                return this.GetDefaultValue<double>(index);
            }

            fixed (byte* pointer = this.userData)
            {
                return *((double*)(pointer + this.GetOffsetForPropertyAtIndex(index)));
            }
        }

        public override ValueReader Clone()
        {
            ByteValueReader clonedReader = (ByteValueReader)this.MemberwiseClone();
            return clonedReader;
        }

        /// <inheritdoc />
        private long ReadInt64AtOffset(int offset)
        {
            fixed (byte* pointer = this.userData)
            {
                return *((long*)(pointer + offset));
            }
        }

        /// <inheritdoc />
        private byte ReadByteAtOffset(int offset)
        {
            fixed (byte* pointer = this.userData)
            {
                return *(pointer + offset);
            }
        }

        /// <summary>
        /// Calculate the offset in the byte array from where the property at given index starts.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        private int GetOffsetForPropertyAtIndex(int index)
        {
            if (index >= this.propertyMetadataWrapperList.Length)
            {
                throw new ArgumentOutOfRangeException("index");
            }

            // First property always starts at 0
            if (index == 0)
            {
                return 0;
            }

            if (this.propertyMetadataWrapperList[index].StartOffset.HasValue)
            {
                return this.propertyMetadataWrapperList[index].StartOffset.Value;
            }

            this.propertyMetadataWrapperList[0].StartOffset = 0;

            // Go over the values till index and calculate the offset. 

            for (int i = 1; i <= index; ++i)
            {
                if (this.propertyMetadataWrapperList[i].StartOffset.HasValue)
                {
                    continue;
                }

                // If a particular Index is not present in the current event, we skip it.
                if (!this.IsIndexAvailableInCurrentVersion(i))
                {
                    continue;
                }

                // Calculate the index of the last field that is present in this Version.
                var indexOfLastFieldPresentInEvent = i - 1;
                for (; indexOfLastFieldPresentInEvent >= 0; --indexOfLastFieldPresentInEvent)
                {
                    if (this.IsIndexAvailableInCurrentVersion(indexOfLastFieldPresentInEvent))
                    {
                        break;
                    }
                }

                // If no field is available, it implies that the current field offset would 0 (i.e. it is the first one present in this event) 
                if (indexOfLastFieldPresentInEvent < 0 )
                {
                    this.propertyMetadataWrapperList[i].StartOffset = 0;
                    continue;
                }

                if (this.propertyMetadataWrapperList[indexOfLastFieldPresentInEvent].Metadata.Type == DataType.String)
                {
                    this.propertyMetadataWrapperList[i].StartOffset =
                        this.SkipUnicodeString(this.propertyMetadataWrapperList[indexOfLastFieldPresentInEvent].StartOffset.GetValueOrDefault());
                }
                else
                {
                    this.propertyMetadataWrapperList[i].StartOffset = this.propertyMetadataWrapperList[indexOfLastFieldPresentInEvent].StartOffset +
                                                                      GetFixedSizes(this.propertyMetadataWrapperList[indexOfLastFieldPresentInEvent].Metadata.Type);
                }
            }

            return this.propertyMetadataWrapperList[index].StartOffset.GetValueOrDefault();
        }

        /// <summary>
        /// Some fields are not available in all versions of an Event. This routine checks if a field at a given index is available in the current event version.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        private bool IsIndexAvailableInCurrentVersion(int index)
        {
            if (index >= this.propertyMetadataWrapperList.Length)
            {
                throw new ArgumentOutOfRangeException("index");
            }

            var attribute = this.propertyMetadataWrapperList[index].Metadata.AttributeData;

            // If version is null, or value is not specified, we assume it's present.
            if (attribute.Version == null || !attribute.Version.HasValue)
            {
                return true;
            }

            // If version is specified and this is less that the current version, index is present
            if (attribute.Version.Value <= this.version)
            {
                return true;
            }

            // It implies that the field at this index is not present in this version of event.
            return false;
        }

        /// <summary>
        /// Gets the default value specified in the attribute for a field.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="index"></param>
        /// <returns></returns>
        private T GetDefaultValue<T>(int index)
        {
            var defaultValue = this.propertyMetadataWrapperList[index].Metadata.AttributeData.DefaultValue;
            if (defaultValue != null)
            {
                return (T)defaultValue;
            }

            throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Event with Version: {0} doesn't include field at Index: {1}", this.version, index));
        }

        /// <summary>
        /// Skip Unicode string starting at 'offset' bytes into the payload blob.
        /// </summary>  
        /// <param name="offset">Start offset</param>
        /// <returns>Offset just after the string</returns>
        private int SkipUnicodeString(int offset)
        {
            while (true)
            {
                fixed (byte* pointer = this.userData)
                {
                    var end = *((short*)(pointer + offset));
                    if (end == 0)
                    {
                        break;
                    }

                    offset += 2;
                }
            }

            offset += 2;
            return offset;
        }

        private static ushort GetFixedSizes(DataType type)
        {
            switch (type)
            {
                case DataType.Int32:
                    return 4;
                case DataType.Guid:
                    return 16;
                case DataType.Byte:
                    return 1;
                case DataType.Ushort:
                    return 2;
                case DataType.Uint:
                    return 4;
                case DataType.Int64:
                    return 8;         
                case DataType.Boolean:
                    // In ETW Events, boolean are DWORD size (4)
                    return 4;
                case DataType.Double:
                    return 8;

                // Date Time is internally stored as long, at least in ETW. 
                // TODO: Will have to check on Linux if this needs an ifdef.
                case DataType.DateTime:
                    return 8;
                default:
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Type: {0} Not supported today", type));
            }
        }

        [DataContract]
        [KnownType(typeof(PropertyMetadataData))]
        private class PropertyMetadataWrapper
        {
            public PropertyMetadataWrapper(PropertyMetadataData data)
            {
                this.Metadata = data;
            }

            [DataMember]
            public PropertyMetadataData Metadata { get; private set; }

            [DataMember]
            public int? StartOffset { get; set; }
        }
    }
}