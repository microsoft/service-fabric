// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Reflection;

    /// <summary>
    /// A Helper class to create Readers, as well as managed a cache of Property Metadata (one copy per Type), for each <see cref="TraceRecord"/> type.
    /// </summary>
    public class ValueReaderCreator
    {
        /// <summary>
        /// A map between type and it's metadata. 
        /// </summary>
        /// <remarks>
        /// There is only a single copy of metadata across assembly and it's only loaded once, irrespective of the number
        /// of actual instances of the Type.
        /// </remarks>
        private static ConcurrentDictionary<Type, PropertyMetadataData[]> typeAndTheirPropertyMetadataMap =
            new ConcurrentDictionary<Type, PropertyMetadataData[]>();


        public static PropertyMetadataData[] GetPropertyMetadataForType(Type objectType)
        {
            return typeAndTheirPropertyMetadataMap.GetOrAdd(objectType, PopulateData(objectType));
        }

        public static ValueReader CreateRawByteValueReader(Type objectType, int version, IntPtr objectData, ushort objectDataLength)
        {
            PropertyMetadataData[] propertyMetadataArray;
            if (typeAndTheirPropertyMetadataMap.TryGetValue(objectType, out propertyMetadataArray))
            {
                return new ByteValueReader(propertyMetadataArray, version, objectData, objectDataLength);
            }

            return new ByteValueReader(typeAndTheirPropertyMetadataMap.GetOrAdd(objectType, PopulateData(objectType)), version, objectData, objectDataLength);
        }

        public static ValueReader CreatePropertyBagValueReader(Type objectType, int version, IDictionary<string, string> properties)
        {
            PropertyMetadataData[] propertyMetadataArray = typeAndTheirPropertyMetadataMap.GetOrAdd(objectType, PopulateData(objectType));
            return new PropertyBagValueReader(propertyMetadataArray, version, properties);
        }

        private static PropertyMetadataData[] PopulateData(Type objectType)
        {
            if (!objectType.IsSubclassOf(typeof(TraceRecord)))
            {
                throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Type: {0} not Supported", objectType));
            }

            var data = new List<PropertyMetadataData>();
            PropertyInfo[] props = objectType.GetProperties();
            foreach (PropertyInfo prop in props)
            {
                object[] attrs = prop.GetCustomAttributes(true);
                data.AddRange(
                    attrs.OfType<TraceFieldAttribute>().Select(
                        traceFieldAttribute => new PropertyMetadataData(ConvertToDataType(prop.PropertyType), prop.Name, traceFieldAttribute)));
            }

            return data.ToArray();
        }

        private static DataType ConvertToDataType(Type type)
        {
            if (type == typeof(int))
            {
                return DataType.Int32;
            }
            else if (type == typeof(uint))
            {
                return DataType.Uint;
            }
            else if (type == typeof(ushort))
            {
                return DataType.Ushort;
            }
            else if (type == typeof(long))
            {
                return DataType.Int64;
            }
            else if (type == typeof(string))
            {
                return DataType.String;
            }
            else if (type == typeof(bool))
            {
                return DataType.Boolean;
            }
            else if (type == typeof(Guid))
            {
                return DataType.Guid;
            }
            else if (type == typeof(DateTime))
            {
                return DataType.DateTime;
            }
            else if (type == typeof(double))
            {
                return DataType.Double;
            }
            else if (type.IsEnum)
            {
                if (type.GetEnumUnderlyingType() == typeof(ushort))
                {
                    return DataType.Ushort;
                }
                else if (type.GetEnumUnderlyingType() == typeof(uint))
                {
                    return DataType.Uint;
                }
                else if (type.GetEnumUnderlyingType() == typeof(int))
                {
                    return DataType.Int32;
                }
                else
                {
                    throw new NotSupportedException(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Underlying Type: {0} of Enum: {1} Not Supported",
                            type.GetEnumUnderlyingType(),
                            type.Name));
                }
            }
            else
            {
                throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Type {0} Not supported", type));
            }
        }
    }
}