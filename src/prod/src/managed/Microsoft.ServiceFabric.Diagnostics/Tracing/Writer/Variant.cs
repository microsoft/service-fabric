// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing.Writer
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Variant class for generic types
    /// </summary>
    internal struct Variant
    {
        private const int SizeOfGuid = 16;
        private readonly ValuesVariant val;
        private readonly string str;
        private readonly byte size;
        private readonly VariantType variantType;

        private Variant(ValuesVariant val, string str, byte size, VariantType variantType)
        {
            this.val = val;
            this.str = str;
            this.size = size;
            this.variantType = variantType;
        }

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

        internal uint Size
        {
            get { return this.size; }
        }

        /// <summary>
        /// Boolean to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(bool value)
        {
            // We use 4B for bool type to match with legacy Windows ETW definition.
            return new Variant(new ValuesVariant { Boolean = value }, null, sizeof(int), VariantType.Boolean);
        }

        /// <summary>
        /// Byte to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(byte value)
        {
            return new Variant(new ValuesVariant { Byte = value }, null, sizeof(byte), VariantType.Byte);
        }

        /// <summary>
        /// SByte to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(sbyte value)
        {
            return new Variant(new ValuesVariant { SByte = value }, null, sizeof(sbyte), VariantType.SByte);
        }

        /// <summary>
        /// Int16 to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(short value)
        {
            return new Variant(new ValuesVariant { Int16 = value }, null, sizeof(short), VariantType.Int16);
        }

        /// <summary>
        /// UInt16 to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(ushort value)
        {
            return new Variant(new ValuesVariant { UInt16 = value }, null, sizeof(ushort), VariantType.UInt16);
        }

        /// <summary>
        /// Int32 to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(int value)
        {
            return new Variant(new ValuesVariant { Int32 = value }, null, sizeof(int), VariantType.Int32);
        }

        /// <summary>
        /// UInt32 to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(uint value)
        {
            return new Variant(new ValuesVariant { UInt32 = value }, null, sizeof(uint), VariantType.UInt32);
        }

        /// <summary>
        /// Int64 to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(long value)
        {
            return new Variant(new ValuesVariant { Int64 = value }, null, sizeof(long), VariantType.Int64);
        }

        /// <summary>
        /// UInt64 to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(ulong value)
        {
            return new Variant(new ValuesVariant { UInt64 = value }, null, sizeof(ulong), VariantType.UInt64);
        }

        /// <summary>
        /// Double to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(double value)
        {
            return new Variant(new ValuesVariant { Double = value }, null, sizeof(double), VariantType.Double);
        }

        /// <summary>
        /// Guid to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(Guid value)
        {
            return new Variant(new ValuesVariant { Guid = value }, null, Variant.SizeOfGuid, VariantType.Guid);
        }

        /// <summary>
        /// DateTime to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(DateTime value)
        {
            return new Variant(
                new ValuesVariant { Int64 = value.ToFileTimeUtc() },
                null,
                sizeof(long),
                VariantType.DateTime);
        }

        /// <summary>
        /// string to Variant
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static implicit operator Variant(string value)
        {
            return new Variant(default(ValuesVariant), value, 0, VariantType.String);
        }

        internal bool IsString()
        {
            return this.variantType == VariantType.String;
        }

        internal string ConvertToString()
        {
            return this.str;
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
                case VariantType.Boolean:
                    return this.val.Boolean;

                case VariantType.Byte:
                    return this.val.Byte;

                case VariantType.SByte:
                    return this.val.SByte;

                case VariantType.Int16:
                    return this.val.Int16;

                case VariantType.UInt16:
                    return this.val.UInt16;

                case VariantType.Int32:
                    return this.val.Int32;

                case VariantType.UInt32:
                    return this.val.UInt32;

                case VariantType.Int64:
                    return this.val.Int64;

                case VariantType.UInt64:
                    return this.val.UInt64;

                case VariantType.Guid:
                    return this.val.Guid;

                case VariantType.DateTime:
                    return DateTime.FromFileTimeUtc(this.val.Int64);

                default:
                    return this.str ?? string.Empty;
            }
        }

        [StructLayout(LayoutKind.Explicit)]
        private struct ValuesVariant
        {
            [FieldOffset(0)] internal bool Boolean;
            [FieldOffset(0)] internal byte Byte;
            [FieldOffset(0)] internal sbyte SByte;
            [FieldOffset(0)] internal short Int16;
            [FieldOffset(0)] internal ushort UInt16;
            [FieldOffset(0)] internal int Int32;
            [FieldOffset(0)] internal uint UInt32;
            [FieldOffset(0)] internal long Int64;
            [FieldOffset(0)] internal ulong UInt64;
            [FieldOffset(0)] internal double Double;
            [FieldOffset(0)] internal Guid Guid;
        }
    }
}