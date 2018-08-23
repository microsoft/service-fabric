// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Runtime.InteropServices;

    internal struct Variant : IEquatable<Variant>
    {
        private const int SizeOfGuid = 16;
        private readonly ValuesVariant val;
        private readonly String str;
        private readonly Byte size;
        private readonly VariantType variantType;

        private enum VariantType : byte
        {
            DefaultVariant = 0,
            Boolean = 1,
            Byte = 2,
            SByte = 3,
            Int16 = 4,
            UInt16 = 5,
            Int32 = 6,
            UInt32 = 7,
            Int64 = 8,
            UInt64 = 9,
            Double = 10,
            Guid = 11,
            DateTime = 12,
            String = 13
        }

        private Variant(ValuesVariant val, String str, Byte size, VariantType variantType)
        {
            this.val = val;
            this.str = str;
            this.size = size;
            this.variantType = variantType;
        }

        internal uint Size
        {
            get { return this.size; }
        }

        public static implicit operator Variant(Boolean value)
        {
            // We use 4B for bool type to match with legacy Windows ETW definition.
            return new Variant(new ValuesVariant { Boolean = value }, null, sizeof(int), VariantType.Boolean);
        }

        public static implicit operator Variant(Byte value)
        {
            return new Variant(new ValuesVariant { Byte = value }, null, sizeof(Byte), VariantType.Byte);
        }

        public static implicit operator Variant(SByte value)
        {
            return new Variant(new ValuesVariant { SByte = value }, null, sizeof(SByte), VariantType.SByte);
        }

        public static implicit operator Variant(Int16 value)
        {
            return new Variant(new ValuesVariant { Int16 = value }, null, sizeof(Int16), VariantType.Int16);
        }

        public static implicit operator Variant(UInt16 value)
        {
            return new Variant(new ValuesVariant { UInt16 = value }, null, sizeof(UInt16), VariantType.UInt16);
        }

        public static implicit operator Variant(Int32 value)
        {
            return new Variant(new ValuesVariant { Int32 = value }, null, sizeof(Int32), VariantType.Int32);
        }

        public static implicit operator Variant(UInt32 value)
        {
            return new Variant(new ValuesVariant { UInt32 = value }, null, sizeof(UInt32), VariantType.UInt32);
        }

        public static implicit operator Variant(Int64 value)
        {
            return new Variant(new ValuesVariant { Int64 = value }, null, sizeof(Int64), VariantType.Int64);
        }

        public static implicit operator Variant(UInt64 value)
        {
            return new Variant(new ValuesVariant { UInt64 = value }, null, sizeof(UInt64), VariantType.UInt64);
        }

        public static implicit operator Variant(Double value)
        {
            return new Variant(new ValuesVariant { Double = value }, null, sizeof(Double), VariantType.Double);
        }

        public static implicit operator Variant(Guid value)
        {
            return new Variant(new ValuesVariant { Guid = value }, null, SizeOfGuid, VariantType.Guid);
        }

        public static implicit operator Variant(DateTime value)
        {
            return new Variant(new ValuesVariant { Int64 = value.ToFileTimeUtc() }, null, sizeof(Int64), VariantType.DateTime);
        }

        public static implicit operator Variant(String value)
        {
            return new Variant(default(ValuesVariant), value, 0, VariantType.String);
        }

        /// <inheritdoc />
        public bool Equals(Variant other)
        {
            if (this.variantType != other.variantType)
            {
                return false;
            }

            switch (this.variantType)
            {
                case VariantType.DefaultVariant: return true;
                case VariantType.String: return this.str.Equals(other.str);
                case VariantType.Boolean: return this.val.Boolean == other.val.Boolean;
                case VariantType.Byte: return this.val.Byte == other.val.Byte;
                case VariantType.SByte: return this.val.SByte == other.val.SByte;
                case VariantType.Int16: return this.val.Int16 == other.val.Int16;
                case VariantType.UInt16: return this.val.UInt16 == other.val.UInt16;
                case VariantType.Int32: return this.val.Int32 == other.val.Int32;
                case VariantType.UInt32: return this.val.UInt32 == other.val.UInt32;
                case VariantType.Int64: return this.val.Int64 == other.val.Int64;
                case VariantType.UInt64: return this.val.UInt64 == other.val.UInt64;
                case VariantType.Double: return this.val.Double.Equals(other.val.Double);
                case VariantType.Guid: return this.val.Guid == other.val.Guid;
                case VariantType.DateTime: return this.val.Int64 == other.val.Int64;

                default:
                    ReleaseAssert.Failfast("Unreachable Code in Variant");
                    break;
            }

            return false;
        }

        public override int GetHashCode()
        {
            switch (this.variantType)
            {
                case VariantType.DefaultVariant: return this.val.GetHashCode();
                case VariantType.String: return this.str.GetHashCode();
                case VariantType.Boolean: return this.val.Boolean.GetHashCode();
                case VariantType.Byte: return this.val.Byte.GetHashCode();
                case VariantType.SByte: return this.val.SByte.GetHashCode();
                case VariantType.Int16: return this.val.Int16.GetHashCode();
                case VariantType.UInt16: return this.val.UInt16.GetHashCode();
                case VariantType.Int32: return this.val.Int32.GetHashCode();
                case VariantType.UInt32: return this.val.UInt32.GetHashCode();
                case VariantType.Int64: return this.val.Int64.GetHashCode();
                case VariantType.UInt64: return this.val.UInt64.GetHashCode();
                case VariantType.Double: return this.val.Double.GetHashCode();
                case VariantType.Guid: return this.val.Guid.GetHashCode();
                case VariantType.DateTime: return this.val.Int64.GetHashCode();

                default:
                    ReleaseAssert.Failfast("Unreachable Code in Variant");
                    break;
            }

            return -1;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return this.ToObject().ToString();
        }

        internal bool IsString()
        {
            return this.variantType == VariantType.String;
        }

        internal bool IsDefaultValue()
        {
            return this.variantType == VariantType.DefaultVariant;
        }

        internal string ConvertToString()
        {
            return this.str;
        }

        internal Boolean ConvertToBoolean()
        {
            return this.val.Boolean;
        }

        internal Byte ConvertToByte()
        {
            return this.val.Byte;
        }

        internal SByte ConvertToSByte()
        {
            return this.val.SByte;
        }

        internal Int16 ConvertToInt16()
        {
            return this.val.Int16;
        }

        internal UInt16 ConvertToUInt16()
        {
            return this.val.UInt16;
        }

        internal Int32 ConvertToInt32()
        {
            return this.val.Int32;
        }

        internal UInt32 ConvertToUInt32()
        {
            return this.val.UInt32;
        }

        internal Int64 ConvertToInt64()
        {
            return this.val.Int64;
        }

        internal UInt64 ConvertToUInt64()
        {
            return this.val.UInt64;
        }

        internal Guid ConvertToGuid()
        {
            return this.val.Guid;
        }

        internal DateTime ConvertToDateTime()
        {
            return DateTime.FromFileTimeUtc(this.val.Int64);
        }

        internal object ToObject()
        {
            switch (this.variantType)
            {
                case VariantType.Boolean: return this.val.Boolean;

                case VariantType.Byte: return this.val.Byte;

                case VariantType.SByte: return this.val.SByte;

                case VariantType.Int16: return this.val.Int16;

                case VariantType.UInt16: return this.val.UInt16;

                case VariantType.Int32: return this.val.Int32;

                case VariantType.UInt32: return this.val.UInt32;

                case VariantType.Int64: return this.val.Int64;

                case VariantType.UInt64: return this.val.UInt64;

                case VariantType.Guid: return this.val.Guid;

                case VariantType.DateTime: return DateTime.FromFileTimeUtc(this.val.Int64);

                default: return this.str ?? string.Empty;
            }
        }

        [StructLayout(LayoutKind.Explicit)]
        private struct ValuesVariant
        {
            [FieldOffset(0)] internal Boolean Boolean;
            [FieldOffset(0)] internal Byte Byte;
            [FieldOffset(0)] internal SByte SByte;
            [FieldOffset(0)] internal Int16 Int16;
            [FieldOffset(0)] internal UInt16 UInt16;
            [FieldOffset(0)] internal Int32 Int32;
            [FieldOffset(0)] internal UInt32 UInt32;
            [FieldOffset(0)] internal Int64 Int64;
            [FieldOffset(0)] internal UInt64 UInt64;
            [FieldOffset(0)] internal Double Double;
            [FieldOffset(0)] internal Guid Guid;
        }
    }
}