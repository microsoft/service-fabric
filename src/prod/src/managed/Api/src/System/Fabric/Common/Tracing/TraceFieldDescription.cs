// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;

    internal struct TraceFieldDescription
    {
        private string name;
        private string inType;
        private string outType;

        private TraceFieldDescription(string name, string inType, string outType)
        {
            this.name = name;
            this.inType = inType;
            this.outType = outType;
        }

        public string Name
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.name;
            }
        }

        public string InType
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.inType;
            }
        }

        public string OutType
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.outType;
            }
        }

        public static TraceFieldDescription Create(string name, Type type, bool isHex)
        {
            if (type == typeof(string))
            {
                return new TraceFieldDescription(name, "win:UnicodeString", "xs:string");
            }
            else if (type == typeof(char))
            {
                return new TraceFieldDescription(name, "win:Int8", "xs:byte");
            }
            else if (type == typeof(byte))
            {
                return new TraceFieldDescription(name, "win:UInt8", "xs:unsignedByte");
            }
            else if (type == typeof(short))
            {
                return new TraceFieldDescription(name, "win:Int16", "xs:short");
            }
            else if (type == typeof(ushort))
            {
                return new TraceFieldDescription(name, "win:UInt16", "xs:unsignedShort");
            }
            else if (type == typeof(int))
            {
                if (isHex)
                {
                    return new TraceFieldDescription(name, "win:HexInt32", "xs:unsignedInt");
                }
                else
                {
                    return new TraceFieldDescription(name, "win:Int32", "xs:int");
                }
            }
            else if (type == typeof(uint))
            {
                if (isHex)
                {
                    return new TraceFieldDescription(name, "win:HexInt32", "xs:unsignedInt");
                }
                else
                {
                    return new TraceFieldDescription(name, "win:UInt32", "xs:unsignedInt");
                }
            }
            else if (type == typeof(long))
            {
                if (isHex)
                {
                    return new TraceFieldDescription(name, "win:HexInt64", "xs:unsignedLong");
                }
                else
                {
                    return new TraceFieldDescription(name, "win:Int64", "xs:long");
                }
            }
            else if (type == typeof(ulong))
            {
                if (isHex)
                {
                    return new TraceFieldDescription(name, "win:HexInt64", "xs:unsignedLong");
                }
                else
                {
                    return new TraceFieldDescription(name, "win:UInt64", "xs:unsignedLong");
                }
            }
            else if (type == typeof(float))
            {
                return new TraceFieldDescription(name, "win:Float", "xs:float");
            }
            else if (type == typeof(double))
            {
                return new TraceFieldDescription(name, "win:Double", "xs:double");
            }
            else if (type == typeof(bool))
            {
                return new TraceFieldDescription(name, "win:Boolean", "xs:boolean");
            }
            else if (type == typeof(Guid))
            {
                return new TraceFieldDescription(name, "win:GUID", "xs:GUID");
            }
            else if (type == typeof(DateTime))
            {
                return new TraceFieldDescription(name, "win:FILETIME", "xs:dateTime");
            }
            else
            {
                throw new ArgumentOutOfRangeException(
                    "type",
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_TypeNotSupported_Formatted,
                        type));
            }
        }
    }
}