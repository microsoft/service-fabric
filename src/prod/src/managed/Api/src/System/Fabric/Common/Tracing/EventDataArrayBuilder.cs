// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Explicit, Size = 16)]
    internal struct EventData
    {
        [FieldOffset(0)]
        internal ulong DataPointer;
        [FieldOffset(8)]
        internal uint Size;
        [FieldOffset(12)]
        internal int Reserved;
    }

    internal unsafe struct EventDataArrayBuilder
    {
        internal const int TraceEventMaximumSize = 65482;
        internal const int BasicTypeAllocationBufferSize = 16;

        private readonly EventData* eventData;

        private uint totalEventSize;
        private EventData* eventDataPtr;
        private byte* currentBuffer;

        internal EventDataArrayBuilder(EventData* eventData, byte* dataBuffer)
        {
            this.totalEventSize = 0;
            this.eventData = eventData;
            this.eventDataPtr = eventData;
            this.currentBuffer = dataBuffer;
        }

        internal void AddEventData(Variant variant)
        {
            // Calculates the total size of the event by adding the data descriptor
            // size value set in EncodeVariant method.
            EventDataArrayBuilder.EncodeVariant(variant, this.eventDataPtr, this.currentBuffer);
            this.currentBuffer += EventDataArrayBuilder.BasicTypeAllocationBufferSize;
            this.totalEventSize += this.eventDataPtr->Size;
            this.eventDataPtr++;
        }

        private void AddStringEventData(Variant variant, int index)
        {
            EventData dataDescriptor = default(EventData);
            dataDescriptor.Reserved = 0;
            if (variant.IsString())
            {
                var str = variant.ConvertToString();
                dataDescriptor.Size = str == null ? 0 : (uint)(str.Length + 1) * 2;
                eventData[index] = dataDescriptor;
                this.totalEventSize += eventData[index].Size;
                this.currentBuffer += EventDataArrayBuilder.BasicTypeAllocationBufferSize;
            }
        }

        // AddEventDataWithTruncation adds EventData by truncating string variants if they exceed expected size.
        // Returns true if EventDataArrayBuilder is in valid state after operation otherwise false.
        internal bool AddEventDataWithTruncation(int argCount, ref Variant v1, ref Variant v2, ref Variant v3,
            ref Variant v4, ref Variant v5, ref Variant v6, ref Variant v7, ref Variant v8, ref Variant v9, 
            ref Variant v10, ref Variant v11, ref Variant v12, ref Variant v13, ref Variant v14)
        {
            AddEventData(v1);
            if (argCount > 1)
            {
                AddEventData(v2);
                if (argCount > 2)
                {
                    AddEventData(v3);
                    if (argCount > 3)
                    {
                        AddEventData(v4);
                        if (argCount > 4)
                        {
                            AddEventData(v5);
                            if (argCount > 5)
                            {
                                AddEventData(v6);
                                if (argCount > 6)
                                {
                                    AddEventData(v7);
                                    if (argCount > 7)
                                    {
                                        AddEventData(v8);
                                        if (argCount > 8)
                                        {
                                            AddEventData(v9);
                                            if (argCount > 9)
                                            {
                                                AddEventData(v10);
                                                if (argCount > 10)
                                                {
                                                    AddEventData(v11);
                                                    if (argCount > 11)
                                                    {
                                                        AddEventData(v12);
                                                        if (argCount > 12)
                                                        {
                                                            AddEventData(v13);
                                                            if (argCount > 13)
                                                            {
                                                                AddEventData(v14);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!IsValid())
            {
                return TruncateStringVariants(ref v1, ref v2, ref v3, ref v4, ref v5, ref v6, ref v7, ref v8, ref v9, ref v10, ref v11, ref v12, ref v13, ref v14);
            }
            return true;
        }

        private void RemoveStringEventData(int index)
        {
            this.currentBuffer -= EventDataArrayBuilder.BasicTypeAllocationBufferSize;
            this.totalEventSize -= eventData[index].Size;
            eventData[index] = default(EventData);
        }

        /// <summary>
        /// Method calculates average characters available for string variants
        /// and then truncates each string variant that is greater than average at the average.
        /// </summary>
        internal bool TruncateStringVariants(ref Variant v0, ref Variant v1, ref Variant v2, ref Variant v3, ref Variant v4, ref Variant v5, ref Variant v6, ref Variant v7, ref Variant v8, ref Variant v9, ref Variant v10, ref Variant v11, ref Variant v12, ref Variant v13)
        {
            bool valid = false;

            // get string variant average length
            var averageStringVariantLength = GetAverageStringVariantLength(ref v0, ref v1, ref v2, ref v3, ref v4, ref v5, ref v6, ref v7, ref v8, ref v9, ref v10, ref v11, ref v12, ref v13);

            // no string variants available for truncation
            if (averageStringVariantLength == 0)
            {
                valid = false;
            }
            else
            {
                TruncateStringVariant(ref v0,  0, averageStringVariantLength);
                TruncateStringVariant(ref v1,  1, averageStringVariantLength);
                TruncateStringVariant(ref v2,  2, averageStringVariantLength);
                TruncateStringVariant(ref v3,  3, averageStringVariantLength);
                TruncateStringVariant(ref v4,  4, averageStringVariantLength);
                TruncateStringVariant(ref v5,  5, averageStringVariantLength);
                TruncateStringVariant(ref v6,  6, averageStringVariantLength);
                TruncateStringVariant(ref v7,  7, averageStringVariantLength);
                TruncateStringVariant(ref v8,  8, averageStringVariantLength);
                TruncateStringVariant(ref v9,  9, averageStringVariantLength);
                TruncateStringVariant(ref v10, 10, averageStringVariantLength);
                TruncateStringVariant(ref v11, 11, averageStringVariantLength);
                TruncateStringVariant(ref v12, 12, averageStringVariantLength);
                TruncateStringVariant(ref v13, 13, averageStringVariantLength);

                valid = IsValid();
            }

            return valid;
        }

        internal EventData* ToEventDataArray(
            char* v0,
            char* v1,
            char* v2,
            char* v3,
            char* v4,
            char* v5,
            char* v6,
            char* v7,
            char* v8,
            char* v9,
            char* v10,
            char* v11,
            char* v12,
            char* v13)
        {
            this.SetStringDataPointers(
            v0, 
            v1, 
            v2,
            v3,
            v4,
            v5,
            v6,
            v7,
            v8,
            v9,
            v10,
            v11,
            v12,
            v13);
            return this.eventData;
        }

        internal bool IsValid()
        {
            return this.totalEventSize <= EventDataArrayBuilder.TraceEventMaximumSize;
        }

        private void SetStringDataPointers(
            char* v0,
            char* v1,
            char* v2,
            char* v3,
            char* v4,
            char* v5,
            char* v6,
            char* v7,
            char* v8,
            char* v9,
            char* v10,
            char* v11,
            char* v12,
            char* v13)
        {
            if (v0 != null)
            {
                this.eventData[0].DataPointer = (ulong)v0;
            }

            if (v1 != null)
            {
                this.eventData[1].DataPointer = (ulong)v1;
            }

            if (v2 != null)
            {
                this.eventData[2].DataPointer = (ulong)v2;
            }

            if (v3 != null)
            {
                this.eventData[3].DataPointer = (ulong)v3;
            }

            if (v4 != null)
            {
                this.eventData[4].DataPointer = (ulong)v4;
            }

            if (v5 != null)
            {
                this.eventData[5].DataPointer = (ulong)v5;
            }

            if (v6 != null)
            {
                this.eventData[6].DataPointer = (ulong)v6;
            }

            if (v7 != null)
            {
                this.eventData[7].DataPointer = (ulong)v7;
            }

            if (v8 != null)
            {
                this.eventData[8].DataPointer = (ulong)v8;
            }

            if (v9 != null)
            {
                this.eventData[9].DataPointer = (ulong)v9;
            }

            if (v10 != null)
            {
                this.eventData[10].DataPointer = (ulong)v10;
            }

            if (v11 != null)
            {
                this.eventData[11].DataPointer = (ulong)v11;
            }

            if (v12 != null)
            {
                this.eventData[12].DataPointer = (ulong)v12;
            }

            if (v13 != null)
            {
                this.eventData[13].DataPointer = (ulong)v13;
            }
        }

        private static void EncodeVariant(Variant data, EventData* dataDescriptor, byte* dataBuffer)
        {
            dataDescriptor->Reserved = 0;
            if (data.IsString())
            {
                var str = data.ConvertToString();
                dataDescriptor->Size = str == null ? 0 : (uint)(str.Length + 1) * 2;
                return;
            }

            // Copy the entire 16B field even if only 1B is being used.
            Guid* guidptr = (Guid*)dataBuffer;
            *guidptr = data.ConvertToGuid();
            dataDescriptor->DataPointer = (ulong)guidptr;
            dataDescriptor->Size = data.Size;
        }

        private int GetAverageStringVariantLength(ref Variant v0, ref Variant v1, ref Variant v2, ref Variant v3, ref Variant v4, ref Variant v5, ref Variant v6, ref Variant v7, ref Variant v8, ref Variant v9, ref Variant v10, ref Variant v11, ref Variant v12, ref Variant v13)
        {
            int count = 0;
            int totalLength = 0;

            if (v0.IsString() && !string.IsNullOrEmpty(v0.ConvertToString()))
            {
                count++;
                totalLength += v0.ConvertToString().Length;
            }

            if (v1.IsString() && !string.IsNullOrEmpty(v1.ConvertToString()))
            {
                count++;
                totalLength += v1.ConvertToString().Length;
            }

            if (v2.IsString() && !string.IsNullOrEmpty(v2.ConvertToString()))
            {
                count++;
                totalLength += v2.ConvertToString().Length;
            }

            if (v3.IsString() && !string.IsNullOrEmpty(v3.ConvertToString()))
            {
                count++;
                totalLength += v3.ConvertToString().Length;
            }

            if (v4.IsString() && !string.IsNullOrEmpty(v4.ConvertToString()))
            {
                count++;
                totalLength += v4.ConvertToString().Length;
            }

            if (v5.IsString() && !string.IsNullOrEmpty(v5.ConvertToString()))
            {
                count++;
                totalLength += v5.ConvertToString().Length;
            }

            if (v6.IsString() && !string.IsNullOrEmpty(v6.ConvertToString()))
            {
                count++;
                totalLength += v6.ConvertToString().Length;
            }

            if (v7.IsString() && !string.IsNullOrEmpty(v7.ConvertToString()))
            {
                count++;
                totalLength += v7.ConvertToString().Length;
            }

            if (v8.IsString() && !string.IsNullOrEmpty(v8.ConvertToString()))
            {
                count++;
                totalLength += v8.ConvertToString().Length;
            }

            if (v9.IsString() && !string.IsNullOrEmpty(v9.ConvertToString()))
            {
                count++;
                totalLength += v9.ConvertToString().Length;
            }

            if (v10.IsString() && !string.IsNullOrEmpty(v10.ConvertToString()))
            {
                count++;
                totalLength += v10.ConvertToString().Length;
            }

            if (v11.IsString() && !string.IsNullOrEmpty(v11.ConvertToString()))
            {
                count++;
                totalLength += v11.ConvertToString().Length;
            }

            if (v12.IsString() && !string.IsNullOrEmpty(v12.ConvertToString()))
            {
                count++;
                totalLength += v12.ConvertToString().Length;
            }

            if (v13.IsString() && !string.IsNullOrEmpty(v13.ConvertToString()))
            {
                count++;
                totalLength += v13.ConvertToString().Length;
            }

            var overflow = GetOverflow();
            var adjustedOverFlow = (overflow % 2 == 1) ? (overflow + 1) : (overflow);
            var characterOverflow = (adjustedOverFlow / 2);

            return (totalLength - characterOverflow) / count;
        }

        private void TruncateStringVariant(ref Variant v, int index, int averageStringVariantLength)
        {
            var str = v.ConvertToString();
            if (!string.IsNullOrEmpty(str) && str.Length > averageStringVariantLength)
            {
                var newVariant = str.Substring(0, averageStringVariantLength);
                RemoveStringEventData(index);
                AddStringEventData(newVariant, index);
                v = newVariant;
            }
        }

        private int GetOverflow()
        {
            var overflow = 0;
            if (!IsValid())
            {
                overflow = (int)this.totalEventSize - EventDataArrayBuilder.TraceEventMaximumSize;
            }
            return overflow;
        }
    }
}