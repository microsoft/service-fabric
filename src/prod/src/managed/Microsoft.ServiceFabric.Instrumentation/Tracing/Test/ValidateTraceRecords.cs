// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;

    internal sealed class ValidateTraceRecords
    {
        private const string ManifestFileName = "Microsoft-ServiceFabric-Events.man";
        private const string TableInfoDelimiter = "__";
        private const char EventInfoDelimiter = '_';

        public static void Validate(string filesDirectory)
        {
            if (!Directory.Exists(filesDirectory))
            {
                throw new DirectoryNotFoundException(filesDirectory);
            }

            // Get details on all the Trace Records we have defined in assembly.
            var typesAndMetadataMap = GetAllTraceRecordsTypes(filesDirectory);

            var fullPathToManifest = Path.Combine(filesDirectory, ManifestFileName);
            if (!File.Exists(fullPathToManifest))
            {
                throw new FileNotFoundException(fullPathToManifest);
            }

            // Load Manifests.
            List<EtwProvider> providers = EtwProvider.LoadProviders(fullPathToManifest);

            int counter = 0;
            foreach (var kvp in typesAndMetadataMap)
            {
                var matchingEventFromManifest = GetEventFromManifest(providers, kvp.Key);
                if (matchingEventFromManifest == null)
                {
                    throw new TraceManifestMismatchException(
                        kvp.Key,
                        MismatchKind.EventMissingInManifest,
                        string.Format("Event ID: {0} missing in manifest", kvp.Key.Identity.TraceId));
                }

                MatchEvents(matchingEventFromManifest, kvp.Key, kvp.Value);
                counter++;
            }

            Console.WriteLine("{0} events validated", counter);
        }

        private static EtwEvent GetEventFromManifest(List<EtwProvider> providers, TraceRecord record)
        {
            EtwEvent manifestEvent = null;
            foreach (var oneProvider in providers)
            {
                manifestEvent = oneProvider.Events.SingleOrDefault(oneEvent => oneEvent.Id == record.Identity.TraceId);
                if (manifestEvent != null)
                {
                    break;
                }
            }

            return manifestEvent;
        }

        private static void MatchEvents(EtwEvent manifestEvent, TraceRecord record, List<ExtendedPropertyInfo> traceRecordMetadata)
        {
            // Validate the mapping which we maintain.
            var mappingValid = IsMappingValid(manifestEvent, record);
            if (!mappingValid)
            {
                throw new TraceManifestMismatchException(record, manifestEvent, MismatchKind.MappingMismatch, "Mapping Mismatch");
            }

            // Validate the Version.
            if (record.GetMinimumVersionOfProductWherePresent() > manifestEvent.Version)
            {
                throw new TraceManifestMismatchException(record, manifestEvent, MismatchKind.EventVersionMismatch, "Event Version Mismatch");
            }

            // Validate Field Count.
            var fields = manifestEvent.Fields;
            if (fields.Count() != traceRecordMetadata.Count())
            {
                throw new TraceManifestMismatchException(
                    record,
                    manifestEvent,
                    MismatchKind.FieldCountDiffer,
                    string.Format("Expected Count: {0}, Actual Count: {1}", traceRecordMetadata.Count, fields.Count));
            }

            // Validate Each Field Type.
            for (int i = 0; i < traceRecordMetadata.Count; ++i)
            {
                // Get the Matching Field.
                var matchingFieldFromManifest = fields.SingleOrDefault(oneField => oneField.Name == traceRecordMetadata[i].ManifestOriginalName);
                if (matchingFieldFromManifest == null)
                {
                    throw new TraceManifestMismatchException(
                        record,
                        manifestEvent,
                        MismatchKind.PropertyMissingInManifest,
                        string.Format("Property Missing. Friendly Name: {0}, Original Name: {1}", traceRecordMetadata[i].Name, traceRecordMetadata[i].ManifestOriginalName));
                }

                // It implies that field is actually an Enum
                if (matchingFieldFromManifest.Enumeration != null)
                {
                    if (!DoesEnumMatch(matchingFieldFromManifest, traceRecordMetadata[i]))
                    {
                        throw new TraceManifestMismatchException(
                            record,
                            manifestEvent,
                            MismatchKind.EnumMismatch,
                            string.Format("Property: {0} in TraceRecord is of Enum Type and has mismatch in manifest", traceRecordMetadata[i].Name));
                    }

                    continue;
                }

                // Validate Position of the Field.
                if (matchingFieldFromManifest.Position != i)
                {
                    throw new TraceManifestMismatchException(
                        record,
                        manifestEvent,
                        MismatchKind.FieldPositionMismatch,
                        string.Format("Expected Position: {0}, Real Position: {1}", i, matchingFieldFromManifest.Position));
                }

                // Validate the Type.
                var manifestDataType = ConvertToDataType(ConvertFromManifestTypeToRealType(matchingFieldFromManifest.Type));
                if (manifestDataType != traceRecordMetadata[i].Type)
                {
                    throw new TraceManifestMismatchException(
                        record,
                        manifestEvent,
                        MismatchKind.DataTypeMismatch,
                        string.Format("Expected type: {0}, Actual Type: {1}", traceRecordMetadata[i].Type, manifestDataType));
                }
            }
        }

        private static bool IsMappingValid(EtwEvent manifestEvent, TraceRecord record)
        {
            var mapping = Mapping.TraceRecordAzureDataMap.SingleOrDefault(item => item.Type == record.GetType());
            if (mapping == null)
            {
                return true;
            }

            // Validate Event Name in the Mapping.
            if (mapping.EventType != manifestEvent.EventName)
            {
                return false;
            }

            // Validate Table Name in Mapping
            var index = manifestEvent.Symbol.IndexOf(TableInfoDelimiter);
            if (index == -1 || (index + TableInfoDelimiter.Length) >= manifestEvent.Symbol.Length)
            {
                return false;
            }

            var symbol = manifestEvent.Symbol.Substring(index + TableInfoDelimiter.Length);
            string[] tableNameParts = symbol.Split(EventInfoDelimiter);
            if (tableNameParts.Length == 0)
            {
                return false;
            }

            if (tableNameParts[0] != mapping.TableName)
            {
                return false;
            }

            return true;
        }

        private static IDictionary<TraceRecord, List<ExtendedPropertyInfo>> GetAllTraceRecordsTypes(string filesDirectory)
        {
            List<Assembly> assemblies = new List<Assembly>();
            foreach (var oneAssembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                assemblies.Add(oneAssembly);
            }

            var dllDirectory = new DirectoryInfo(filesDirectory);
            FileInfo[] dlls = dllDirectory.GetFiles("*.dll");
            foreach (FileInfo dllFileInfo in dlls)
            {
                Assembly assembly = Assembly.LoadFile(dllFileInfo.FullName);
                if (!assemblies.Contains(assembly))
                {
                    assemblies.Add(assembly);
                }
            }

            var knownTypes = new List<Type>();
            foreach (var oneAssembly in assemblies)
            {
                try
                {
                    var types = oneAssembly.GetTypes();
                    knownTypes.AddRange(types.Where(type => type.IsSubclassOf(typeof(TraceRecord)) && !type.IsAbstract));

                    foreach (var oneType in types)
                    {
                        var one = oneType.IsAssignableFrom(typeof(TraceRecord));
                        var two = oneType.IsSubclassOf(typeof(TraceRecord));
                    }
                }
                catch (Exception)
                {
                }
            }

            if (!knownTypes.Any())
            {
                throw new Exception("Error : No known Types found to validate");
            }

            var typeAndMetadataMap = new Dictionary<TraceRecord, List<ExtendedPropertyInfo>>();
            foreach (var oneType in knownTypes)
            {
                typeAndMetadataMap[(TraceRecord)Activator.CreateInstance(oneType, new object[] { null })] = RetrieveExtendedPropertyInformation(oneType).OrderBy(item => item.ManifestIndex).ToList();
            }

            return typeAndMetadataMap;
        }

        private static bool DoesEnumMatch(Field field, ExtendedPropertyInfo propertyInfo)
        {
            if (field.Enumeration == null)
            {
                return false;
            }

            // Known Bug in PLB Scheduler. ErrorCode changes too often and I need to do some thinking on what is the right set of validation
            // for such an Enum
            if (propertyInfo.RealType.Name == "PLBSchedulerActionType" || propertyInfo.RealType.Name == "ErrorCodeValue")
            {
                return true;
            }

            if (!propertyInfo.RealType.IsEnum)
            {
                return false;
            }

            var enumerations = field.Enumeration;

            var enumValueFromTraceRecord = Enum.GetValues(propertyInfo.RealType).Cast<int>().ToDictionary(e => e, e => Enum.GetName(propertyInfo.RealType, e));
            var enumValuesFromManifest = field.Enumeration.Values;

            // Make sure both enums definitions have same number of fields.
            if (enumValueFromTraceRecord.Count != enumValuesFromManifest.Count())
            {
                return false;
            }

            foreach (var oneManifestValue in enumValuesFromManifest)
            {
                if (!enumValueFromTraceRecord.Contains(oneManifestValue))
                {
                    return false;
                }
            }

            return true;
        }

        private static ExtendedPropertyInfo[] RetrieveExtendedPropertyInformation(Type objectType)
        {
            if (!objectType.IsSubclassOf(typeof(TraceRecord)))
            {
                throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Type: {0} not Supported", objectType));
            }

            var data = new List<ExtendedPropertyInfo>();
            PropertyInfo[] props = objectType.GetProperties();
            foreach (PropertyInfo prop in props)
            {
                object[] attrs = prop.GetCustomAttributes(true);
                data.AddRange(
                    attrs.OfType<TraceFieldAttribute>().Select(
                        traceFieldAttribute => new ExtendedPropertyInfo
                        {
                            Type = ConvertToDataType(prop.PropertyType),
                            Name = prop.Name,
                            RealType = prop.PropertyType,
                            ManifestOriginalName = traceFieldAttribute.OriginalName,
                            ManifestIndex = traceFieldAttribute.Index,
                            ManifestVersion = traceFieldAttribute.Version.GetValueOrDefault(1),
                        }));
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

        private static Type ConvertFromManifestTypeToRealType(string manifestType)
        {
            switch (manifestType)
            {
                // Booleans are DWORD sized.
                case "win:Boolean":
                    return typeof(bool);

                case "win:UInt8":
                case "win:HexInt8":
                case "win:Int8":
                    return typeof(int);

                case "win:UInt16":
                case "win:HexInt16":
                case "win:Int16":
                case "trace:Port":
                    return typeof(int);

                case "win:UInt32":
                    return typeof(uint);

                case "win:HexInt32":
                case "win:Int32":
                case "trace:IPAddr":
                case "trace:IPAddrV4":
                    return typeof(int);

                case "win:Double":
                    return typeof(double);

                case "win:Float":
                    return typeof(float);

                case "trace:WmiTime":
                case "win:HexInt64":
                case "win:UInt64":
                case "win:Int64":
                    return typeof(long);

                case "trace:UnicodeChar":
                case "win:UnicodeString":
                case "win:AnsiString":
                    return typeof(string);

                case "win:GUID":
                case "trace:Guid":
                    return typeof(Guid);

                case "win:FILETIME":
                    return typeof(DateTime);

                case "win:Binary":
                    return typeof(byte[]);

                default:
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Type: {0} Not supported", manifestType));
            }
        }
    }
}