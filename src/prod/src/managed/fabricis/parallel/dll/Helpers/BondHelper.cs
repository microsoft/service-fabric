// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Bond;
    using Bond.IO.Safe;
    using Bond.Protocols;

    public static class BondHelper
    {
        /// <remarks>
        /// TODO - fix this
        /// This method exists since the bond.dll provided implicit converter is failing with
        /// Method not found exception: System.Guid Bond.GUID.op_Implicit(Bond.GUID)
        /// So, taken this code from bond.dll.
        /// Reference: https://github.com/Microsoft/bond/pull/145
        /// </remarks>
        public static GUID ToGUID(this Guid systemGuid)
        {
            var bytes = systemGuid.ToByteArray();
            return new GUID
            {
                Data1 = BitConverter.ToUInt32(bytes, 0),
                Data2 = BitConverter.ToUInt16(bytes, 4),
                Data3 = BitConverter.ToUInt16(bytes, 6),
                Data4 = BitConverter.ToUInt64(bytes, 8),
            };
        }

        /// <remarks>
        /// <see cref="ToGUID(Guid)"/> for reason for this method.
        /// </remarks>
        public static Guid ToGuid(this GUID bondGuid)
        {
            return new Guid(
                (int)bondGuid.Data1,
                (short)bondGuid.Data2,
                (short)bondGuid.Data3,
                (byte)(bondGuid.Data4 >> 0),
                (byte)(bondGuid.Data4 >> 8),
                (byte)(bondGuid.Data4 >> 16),
                (byte)(bondGuid.Data4 >> 24),
                (byte)(bondGuid.Data4 >> 32),
                (byte)(bondGuid.Data4 >> 40),
                (byte)(bondGuid.Data4 >> 48),
                (byte)(bondGuid.Data4 >> 56));
        }

        public static GUID ToGUID(this string guid)
        {
            guid.Validate("guid");

            Guid systemGuid;
            bool success = Guid.TryParse(guid, out systemGuid);

            if (!success)
            {
                Constants.TraceType.WriteWarning("Unable to parse string into a GUID. String: {0}", guid);
                return null;
            }

            GUID bondGuid = systemGuid.ToGUID();
            return bondGuid;
        }

        public static T GetBondObjectFromPayload<T>(this byte[] requestPayload) where T : class, new()
        {
            requestPayload.Validate("requestPayload");

            var input = new InputBuffer(requestPayload);
            var reader = new CompactBinaryReader<InputBuffer>(input);
            T result = Deserialize<T>.From(reader);

            return result;
        }

        public static byte[] GetPayloadFromBondObject<T>(this T bondObject) where T : class, new()
        {
            bondObject.Validate("bondObject");

            var output = new OutputBuffer();
            var writer = new CompactBinaryWriter<OutputBuffer>(output);
            Serialize.To(writer, bondObject);

            return output.Data.Array;
        }
    }
}