// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

using System.Fabric.Common;

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;
    using TraceDiagnostic.Store;

    /// <summary>
    /// Helper methods to instantiate event entities; the local-store counterpart of the EntityHelper in TraceDiagnostic.Store.
    /// </summary>
    internal static class EntityHelperLocal
    {
        /// <summary>
        /// Cache for looking up the TypeConverter for different types
        /// </summary>
        private static readonly Dictionary<Type, TypeConverter> TypeConverterCache = new Dictionary<Type, TypeConverter>();

        /// <summary>
        /// Cache for looking up the PropertyInfo of "fieldName" properties for different types.
        /// </summary>
        private static Dictionary<Type, Dictionary<string, PropertyInfo>> typePropertyCache = new Dictionary<Type, Dictionary<string, PropertyInfo>>();

        /// <summary>
        /// Maps (eventType, taskName) ordered pair to the corresponding event class type
        /// </summary>
        private static readonly Dictionary<Tuple<string, string>, Type> eventTypes = new Dictionary<Tuple<string, string>, Type>()
        {
            { Tuple.Create(EventTypes.ApiError, TaskNames.RAP),                                 typeof(TraceDiagnostic.Store.ApiErrorEvent) },
            { Tuple.Create(EventTypes.ApiSlow, TaskNames.RAP),                                  typeof(TraceDiagnostic.Store.ApiFaultEvent) },
            { Tuple.Create(EventTypes.HostedServiceUnexpectedTermination, TaskNames.Hosting),   typeof(TraceDiagnostic.Store.UnexpectedTerminationEvent) },
            { Tuple.Create(EventTypes.ProcessUnexpectedTermination, TaskNames.Hosting),         typeof(TraceDiagnostic.Store.UnexpectedTerminationEvent) }
        };

        /// <summary>
        /// Maps event class type to the corresponding default constructor delegate
        /// </summary>
        private static Dictionary<Tuple<string, string>, Func<object>> ctorsOfRegisteredEvents;

        /// <summary>
        /// Gets the constructor delegate for an event
        /// </summary>
        private static Dictionary<Tuple<string, string>, Func<object>> EventConstructors
        {
            get
            {
                if (ctorsOfRegisteredEvents == null)
                {
                    ctorsOfRegisteredEvents = LoadEventTypes();
                }

                return ctorsOfRegisteredEvents;
            }
        }

        /// <summary>
        /// Creates a derived NodeEvent object or a NodeEvent object from a trace record
        /// </summary>
        /// <param name="eventType">Type of event that the record belongs to</param>
        /// <param name="partitionKey">The PartitionKey of the record</param>
        /// <param name="time">The timestamp of the record</param>
        /// <param name="properties">The captured properties of the trace record</param>
        /// <returns>NodeEvent object</returns>
        public static NodeEvent CreateNodeEvent(
            string eventType,
           
            string partitionKey,
            DateTime time,
            IDictionary<string, string> properties)
        {
            var taskName = properties[FieldNames.Source];
            var uniquePair = Tuple.Create(eventType, taskName);

            var item = EventConstructors.ContainsKey(uniquePair)
                ? (NodeEvent)EventConstructors[uniquePair]()
                : new NodeEvent();

            item.NodeId = partitionKey;
            item.Time = time;

            SetPropertyValue(item, properties);

            return item;
        }

        /// <summary>
        /// Creates a derived PartitionEvent object or a PartitionEvent object from a trace record
        /// </summary>
        /// <param name="eventType">Type of event that the record belongs to</param>
        /// <param name="partitionKey">The PartitionKey of the record</param>
        /// <param name="time">The timestamp of the record</param>
        /// <param name="properties">The captured properties of the trace record</param>
        /// <returns>PartitionEvent object</returns>
        public static PartitionEvent CreatePartitionEvent(
            string eventType,
            string partitionKey,
            DateTime time,
            IDictionary<string, string> properties)
        {
            var taskName = properties[FieldNames.Source];
            var uniquePair = Tuple.Create(eventType, taskName);

            var item = EventConstructors.ContainsKey(uniquePair)
                ? (PartitionEvent)EventConstructors[uniquePair]()
                : new PartitionEvent();

            item.PartitionId = partitionKey;
            item.Time = time;

            if (properties != null)
            {
                SetPropertyValue(item, properties);
                item.SecondaryNodes = ExtractArrayProperty(properties, FieldNames.Secondaries);
                item.Instances = ExtractArrayProperty(properties, FieldNames.Instances);
            }

            return item;
        }

        /// <summary>
        /// Sets the property values for the given object from specified property bag.
        /// </summary>
        /// <param name="item">The object to set properties to.</param>
        /// <param name="propertyBag">The property bag from where the values are loaded.</param>
        public static void SetPropertyValue(object item, IDictionary<string, string> propertyBag)
        {
            if (propertyBag == null || item == null)
            {
                return;
            }

            var properties = GetFieldProperties(item.GetType());
            if (properties == null)
            {
                return;
            }

            foreach (var kv in propertyBag)
            {
                if (properties.ContainsKey(kv.Key))
                {
                    var val = kv.Value.GetValue(properties[kv.Key].PropertyType);

                    PropertyInfo prop = null;
                    if (properties.TryGetValue(kv.Key, out prop))
                    {
                        prop.SetValue(item, val, null);
                    }
                }
            }
        }

        /// <summary>
        /// For a specific Type type, gets all its properties which have been marked as "FieldName(xxx)"
        /// </summary>
        /// <param name="type">The type to get field properties for.</param>
        /// <returns>List of properties indexed by their corresponding field name.</returns>
        private static Dictionary<string, PropertyInfo> GetFieldProperties(Type type)
        {
            if (typePropertyCache.ContainsKey(type))
            {
                return typePropertyCache[type];
            }

            var dict = new Dictionary<string, PropertyInfo>(StringComparer.OrdinalIgnoreCase);
            foreach (var prop in type.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.SetProperty))
            {
                var attributes = prop.GetCustomAttributes(true);
                if (attributes != null)
                {
                    var fieldNameAttr = attributes.FirstOrDefault(a => a is FieldNameAttribute);
                    if (fieldNameAttr != null)
                    {
                        var fieldName = (FieldNameAttribute)fieldNameAttr;
                        dict[fieldName.Name] = prop;
                    }
                }
            }

            typePropertyCache[type] = dict;

            return dict;
        }

        /// <summary>
        /// Extracts a space delimited property into a string array, if it exists in the specified property bag.
        /// </summary>
        /// <param name="properties">The property bag.</param>
        /// <param name="fieldName">The name of the field.</param>
        /// <returns>String array extracted from the property.</returns>
        private static string[] ExtractArrayProperty(IDictionary<string, string> properties, string fieldName)
        {
            if (properties.ContainsKey(fieldName))
            {
                string items = properties[fieldName];
                if (!string.IsNullOrEmpty(items))
                {
                    return items.Split(' ');
                }
            }

            return null;
        }

        /// <summary>
        /// Coverts the string stringValue into an object of Type type
        /// </summary>
        /// <param name="stringValue">Value of the object as string</param>
        /// <param name="type">Type of the object to which the stringValue needs to be converted to</param>
        /// <returns>An object of Type type</returns>
        private static object GetValue(this string stringValue, Type type)
        {
            if (!TypeConverterCache.ContainsKey(type))
            {
                TypeConverterCache[type] = TypeDescriptor.GetConverter(type);
            }

            return TypeConverterCache[type].ConvertFromString(stringValue);
        }

        /// <summary>
        /// For a given Type type, creates a default-constructor delegate.
        /// </summary>
        /// <param name="type">The type for which a default constructor delegate needs to be created</param>
        /// <returns>Default-constructor delegate object</returns>
        private static Func<object> CreateDefaultConstructor(Type type)
        {
            NewExpression defaultCtor = Expression.New(type);

            var lambda = Expression.Lambda<Func<object>>(defaultCtor);

            return lambda.Compile();
        }

        /// <summary>
        /// Caches a constructor-delegate for each derived event
        /// </summary>
        private static Dictionary<Tuple<string, string>, Func<object>> LoadEventTypes()
        {
            var eventConstructors = new Dictionary<Tuple<string, string>, Func<object>>();

            foreach (var kvp in eventTypes)
            {
                eventConstructors.Add(kvp.Key, CreateDefaultConstructor(kvp.Value));
            }

            return eventConstructors;
        }
    }
}

#pragma warning restore 1591