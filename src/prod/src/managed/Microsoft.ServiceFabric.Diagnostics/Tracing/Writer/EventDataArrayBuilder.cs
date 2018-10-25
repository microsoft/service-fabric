// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing.Writer
{
    using System;
    using System.Runtime.InteropServices;

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
            // Caculates the total size of the event by adding the data descriptor
            // size value set in EncodeVariant method.
            this.EncodeVariant(variant, this.eventDataPtr, this.currentBuffer);
            this.currentBuffer += EventDataArrayBuilder.BasicTypeAllocationBufferSize;
            this.totalEventSize += this.eventDataPtr->Size;
            this.eventDataPtr++;
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
            char* v8)
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
            v8);
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
            char* v8)
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
        }

        private void EncodeVariant(Variant data, EventData* dataDescriptor, byte* dataBuffer)
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
    }

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
}