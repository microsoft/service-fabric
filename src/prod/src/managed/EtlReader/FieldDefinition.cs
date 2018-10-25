// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;

    public class FieldDefinition
    {
        private string name;
        private FieldType inType;
        private string length;
        private string mapName;
        private bool isContextField;
        private bool wellKnownField;
        private bool omitIfZero;
        private int encodedFieldWidth;
        private bool skipFormatting;

        private static readonly string ConsequenceFieldPrefix = "contextSequenceId_";
        private static readonly string ConsequenceIdField = "contextSequenceId";
        private static readonly string OmitZeroFieldFlag = "_omitZero_";
        private static readonly string SpecialFieldTypePrefix = "_type:";

        // Maps special field types to the number of fields it consists of
        private static readonly Dictionary<FieldType, int> SpecialFieldLength = new Dictionary<FieldType, int>
        {
            { FieldType.LargeInteger, 2 }
        };

        public FieldDefinition(string name, string inType, string length, string mapName, bool wellKnownField, bool skipFormatting)
        {
            this.name = name;
            this.inType = FieldDefinition.GetFieldTypeFromString(inType);
            this.length = length;
            this.mapName = mapName;
            this.wellKnownField = wellKnownField;
            this.skipFormatting = skipFormatting;

            if (name != null)
            {
                this.isContextField = (this.wellKnownField &&
                                       name.StartsWith(ConsequenceFieldPrefix, StringComparison.OrdinalIgnoreCase) &&
                                       this.inType == FieldType.UInt16);
            }

            this.omitIfZero = name.Contains(OmitZeroFieldFlag);
            this.encodedFieldWidth = 1;

            if (name.Contains(SpecialFieldTypePrefix))
            {
                string specialTypeName = name.Substring(name.IndexOf(SpecialFieldTypePrefix) + SpecialFieldTypePrefix.Length);
                specialTypeName = specialTypeName.Substring(0, specialTypeName.IndexOf('_'));
                FieldType specialType = FieldDefinition.GetSpecialFieldTypeFromString(specialTypeName);
                if (specialType != FieldType.Unknown)
                {
                    this.inType = specialType;
                    this.encodedFieldWidth = SpecialFieldLength[specialType];
                }
            }
        }

        public string Name
        {
            get
            {
                return name;
            }
        }

        public FieldType Type
        {
            get
            {
                return inType;
            }
        }

        public bool IsContextField
        {
            get
            {
                return isContextField;
            }
        }

        public bool IsContextId
        {
            get
            {
                return (this.wellKnownField && name == ConsequenceIdField && this.inType == FieldType.UInt16);
            }
        }

        public int EncodedFieldWidth
        {
            get
            {
                return encodedFieldWidth;
            }
        }

        public string FormatField(ApplicationDataReader reader, ValueMaps valueMaps)
        {
            return this.FormatField(CultureInfo.InvariantCulture, reader, valueMaps);
        }

        public string FormatField(IFormatProvider provider, ApplicationDataReader reader, ValueMaps valueMaps)
        {
            if (provider == null)
            {
                throw new ArgumentNullException("provider");
            }

            if (reader == null)
            {
                throw new ArgumentNullException("reader");
            }

            if (valueMaps == null)
            {
                throw new ArgumentNullException("valueMaps");
            }

            if (this.skipFormatting)
            {
                return "";
            }

            try
            {
                switch (this.inType)
                {
                    case FieldType.UnicodeString:
                        return reader.ReadUnicodeString();

                    case FieldType.AnsiString:
                        return reader.ReadAnsiString();

                    case FieldType.Boolean:
                        return reader.ReadBoolean().ToString();

                    case FieldType.UInt8:
                        return reader.ReadUInt8().ToString(provider);

                    case FieldType.UInt16:
                        return reader.ReadUInt16().ToString(provider);

                    case FieldType.UInt32:
                        uint key = reader.ReadUInt32();

                        if (!string.IsNullOrEmpty(this.mapName))
                        {
                            string stringValue = null;
                            if (valueMaps.TryGetValue(this.mapName, key, out stringValue))
                            {
                                return stringValue;
                            }
                        }

                        return key.ToString(provider);

                    case FieldType.UInt64:
                        UInt64 value = reader.ReadUInt64();

                        if (this.omitIfZero && value == 0)
                        {
                            return System.String.Empty;
                        }

                        return value.ToString(provider);

                    case FieldType.Int8:
                        return reader.ReadInt8().ToString(provider);

                    case FieldType.Int16:
                        return reader.ReadInt16().ToString(provider);

                    case FieldType.Int32:
                        return reader.ReadInt32().ToString(provider);

                    case FieldType.Int64:
                        return reader.ReadInt64().ToString(provider);

                    case FieldType.HexInt32:
                        return reader.ReadUInt32().ToString("x", provider);

                    case FieldType.HexInt64:
                        value = reader.ReadUInt64();

                        if (this.omitIfZero && value == 0)
                        {
                            return System.String.Empty;
                        }

                        return value.ToString("x", provider);

                    case FieldType.Float:
                        return reader.ReadFloat().ToString(provider);

                    case FieldType.Double:
                        return reader.ReadDouble().ToString(provider);

                    case FieldType.DateTime:
                        return reader.ReadFileTime().ToString("yyyy-M-d HH:mm:ss.fff", provider);

                    case FieldType.Guid:
                        return reader.ReadGuid().ToString("D");

                    case FieldType.LargeInteger:
                        UInt64 highValue = reader.ReadUInt64();
                        UInt64 lowValue = reader.ReadUInt64();

                        if (highValue == 0)
                        {
                            return lowValue.ToString("x", provider);
                        }

                        return highValue.ToString("x", provider) + lowValue.ToString("x16", provider);

                    default:
                        return "<unknown type " + this.inType + ">";
                }
            }
            catch (OverflowException)
            {
                return "<unknown " + this.inType + " value>";
            }
            catch (ArgumentException e)
            {
                if (!string.IsNullOrEmpty(Environment.GetEnvironmentVariable("WeakETLReader")))
                {
                    return string.Format("(!!! Invalid value for field {0}: {1} !!!)", this.Name, e.Message);
                }
                throw;
            }
        }

        private static FieldType GetFieldTypeFromString(string inType)
        {
            switch (inType)
            {
                case "win:UnicodeString":
                    return FieldType.UnicodeString;

                case "win:AnsiString":
                    return FieldType.AnsiString;

                case "win:Boolean":
                    return FieldType.Boolean;

                case "win:UInt8":
                    return FieldType.UInt8;

                case "win:UInt16":
                    return FieldType.UInt16;

                case "win:UInt32":
                    return FieldType.UInt32;

                case "win:UInt64":
                    return FieldType.UInt64;

                case "win:Int8":
                    return FieldType.Int8;

                case "win:Int16":
                    return FieldType.Int16;

                case "win:Int32":
                    return FieldType.Int32;

                case "win:Int64":
                    return FieldType.Int64;

                case "win:HexInt32":
                    return FieldType.HexInt32;

                case "win:HexInt64":
                    return FieldType.HexInt64;

                case "win:Float":
                    return FieldType.Float;

                case "win:Double":
                    return FieldType.Double;

                case "win:FILETIME":
                    return FieldType.DateTime;

                case "win:GUID":
                    return FieldType.Guid;

                case "win:Pointer":
                    return FieldType.HexInt64;

                default:
                    return FieldType.Unknown;
            }
        }

        private static FieldType GetSpecialFieldTypeFromString(string specialTypeName)
        {
            switch (specialTypeName)
            {
                case "LargeInteger":
                    return FieldType.LargeInteger;

                default:
                    return FieldType.Unknown;
            }
        }

        public enum FieldType
        {
            UnicodeString,
            AnsiString,
            Boolean,
            UInt8,
            UInt16,
            UInt32,
            UInt64,
            Int8,
            Int16,
            Int32,
            Int64,
            HexInt32,
            HexInt64,
            Float,
            Double,
            DateTime,
            Guid,
            LargeInteger,
            Unknown
        }
    }
}