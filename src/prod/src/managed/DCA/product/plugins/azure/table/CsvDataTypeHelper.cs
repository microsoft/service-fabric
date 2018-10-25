// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Reflection;
    using System.Text.RegularExpressions;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    // This is a utility class for Azure table uploaders. 
    internal class CsvDataTypesHelper
    {
        private class FieldInfo
        {
            public FieldType fieldType;
            public Regex parseRegex;
        }

        private Dictionary<char, FieldInfo> typeMap = new Dictionary<char, FieldInfo>()
        {
            { 'b', new FieldInfo() { fieldType = FieldType.UInt8, parseRegex = new Regex(@"[0-9]{1,3}") } },
            { 'B', new FieldInfo() { fieldType = FieldType.Int8, parseRegex = new Regex(@"-?[0-9]{1,3}") } },
            { 'd', new FieldInfo() { fieldType = FieldType.DateTime, parseRegex = new Regex(@".*") } },
            { 'D', new FieldInfo() { fieldType = FieldType.Double, parseRegex = new Regex(@"[0-9]+\.?[0-9]*") } },
            { 'G', new FieldInfo() { fieldType = FieldType.Guid, parseRegex = new Regex(@"\{?[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}\}?")} },
            { 'h', new FieldInfo() { fieldType = FieldType.HexInt32, parseRegex = new Regex(@"[0-9A-Fa-f]{1,8}") } },
            { 'H', new FieldInfo() { fieldType = FieldType.HexInt64, parseRegex = new Regex(@"[0-9A-Fa-f]{1,16}") } },
            { 'i', new FieldInfo() { fieldType = FieldType.UInt32, parseRegex = new Regex(@"[0-9]{1,10}") } },
            { 'I', new FieldInfo() { fieldType = FieldType.Int32, parseRegex = new Regex(@"-?[0-9]{1,10}") } },
            { 'l', new FieldInfo() { fieldType = FieldType.UInt64, parseRegex = new Regex(@"[0-9]{1,20}") } },
            { 'L', new FieldInfo() { fieldType = FieldType.Int64, parseRegex = new Regex(@"-?[0-9]{1,19}") } },
            { 's', new FieldInfo() { fieldType = FieldType.UInt16, parseRegex = new Regex(@"[0-9]{1,5}") } },
            { 'S', new FieldInfo() { fieldType = FieldType.UnicodeString, parseRegex = new Regex(@".*") } },
            { 'T', new FieldInfo() { fieldType = FieldType.TimeSpan, parseRegex = new Regex(@".*") } },
            { 'x', new FieldInfo() { fieldType = FieldType.Boolean, parseRegex = new Regex(@"true|false|True|False") } },
        };

        public object GetValueFromIdentifier(char id, string input, out FieldType fieldType)
        {
            var fieldInfo = this.typeMap[id];
            var match = fieldInfo.parseRegex.Match(input);

            if (!match.Success)
            {
                var message = "Invalid values id:" + id.ToString() + " input:" + input + " regex:" + fieldInfo.parseRegex;
                throw new ArgumentException(message);
            }

            fieldType = fieldInfo.fieldType;
            return match.Value;
        }
        
        public enum FieldType
        {
            AnsiString,
            Boolean,
            DateTime,
            Double,
            Float,
            HexInt32,
            HexInt64,
            Int8,
            Int16,
            Int32,
            Int64,
            Guid,
            TimeSpan,
            UInt8,
            UInt16,
            UInt32,
            UInt64,
            UnicodeString,
            Unknown
        }
    }
}