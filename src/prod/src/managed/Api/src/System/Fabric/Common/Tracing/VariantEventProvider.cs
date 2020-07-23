// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Diagnostics;
    using System.Diagnostics.Eventing;

    internal class VariantEventProvider : EventProvider, IVariantEventWriter
    {
        internal VariantEventProvider(Guid providerGuid)
            : base(providerGuid)
        {
        }

        public unsafe void VariantWrite(
            ref GenericEventDescriptor genericEventDescriptor,
            int argCount,
            Variant v0 = default(Variant),
            Variant v1 = default(Variant),
            Variant v2 = default(Variant),
            Variant v3 = default(Variant),
            Variant v4 = default(Variant),
            Variant v5 = default(Variant),
            Variant v6 = default(Variant),
            Variant v7 = default(Variant),
            Variant v8 = default(Variant),
            Variant v9 = default(Variant),
            Variant v10 = default(Variant),
            Variant v11 = default(Variant),
            Variant v12 = default(Variant),
            Variant v13 = default(Variant))
        {
            EventDescriptor eventDesc = new EventDescriptor(genericEventDescriptor.EventId, genericEventDescriptor.Version, genericEventDescriptor.Channel, genericEventDescriptor.Level, genericEventDescriptor.Opcode, genericEventDescriptor.Task, genericEventDescriptor.Keywords);

            if (argCount == 0)
            {
                this.WriteEvent(ref eventDesc, argCount, null);
                return;
            }

            EventData* eventSourceData = stackalloc EventData[argCount]; // allocation for the data descriptors
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize*argCount];
            
            // 16 byte for non-string argument
            var edb = new EventDataArrayBuilder(eventSourceData, dataBuffer);

            if (!edb.AddEventDataWithTruncation(argCount, ref v0,ref v1, ref v2, ref v3, ref v4, ref v5, ref v6, ref v7, ref v8, ref v9, ref v10, ref v11, ref v12, ref v13))
            {
                Debug.Fail(string.Format("EventData for eventid {0} is invalid. Check the total size of the event.",
                    eventDesc.EventId));
                return;
            }

            fixed (
                char* s0 = v0.ConvertToString(),
                s1  = v1.ConvertToString(),
                s2  = v2.ConvertToString(),
                s3  = v3.ConvertToString(),
                s4  = v4.ConvertToString(),
                s5  = v5.ConvertToString(),
                s6  = v6.ConvertToString(),
                s7  = v7.ConvertToString(),
                s8  = v8.ConvertToString(),
                s9  = v9.ConvertToString(),
                s10 = v10.ConvertToString(),
                s11 = v11.ConvertToString(),
                s12 = v12.ConvertToString(),
                s13 = v13.ConvertToString())
            {
                var eventDataPtr = edb.ToEventDataArray(
                    s0,
                    s1,
                    s2,
                    s3,
                    s4,
                    s5,
                    s6,
                    s7,
                    s8,
                    s9,
                    s10,
                    s11,
                    s12,
                    s13);
                this.WriteEvent(ref eventDesc, argCount, (IntPtr) eventDataPtr);
            }
        }
    }
}