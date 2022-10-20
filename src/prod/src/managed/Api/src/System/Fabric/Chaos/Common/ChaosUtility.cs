// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.Common
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Chaos.RandomActionGenerator;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Reflection;
    using System.Text;

    internal static class ChaosUtility
    {
        public const string TraceType = "ChaosUtility";
        public const string IEnumerableInterfaceName = "IEnumerable";
        public const char ListElementSeparator = ';';
        public const char NamespaceSeparator = '.';

        /// <summary>
        /// Whether to update a value of a property on the object or verify that a property on the object has a certain value.
        /// </summary>
        public enum ObjectVisitMode
        {
            Update = 0,
            Verify = 1
        }

        public static bool EngineAssertEnabled { get; set; }
        public static bool ForcedEngineAssertEnabled { get; set; }
        public static bool ForcedMoveOfReplicaEnabled { get; set; }

        public static bool DisableOptimizationForValidationFailedEvent { get; set; }

        public static Dictionary<Type, int> ChaosEventTypeToIntMap = new Dictionary<Type, int>()
                {
                    {typeof(StartedEvent),                 (int)ChaosEventType.Started},
                    {typeof(StoppedEvent),                 (int)ChaosEventType.Stopped},
                    {typeof(ExecutingFaultsEvent),         (int)ChaosEventType.ExecutingFaults},
                    {typeof(ValidationFailedEvent),        (int)ChaosEventType.ValidationFailed},
                    {typeof(TestErrorEvent),               (int)ChaosEventType.TestError},
                    {typeof(WaitingEvent),                 (int)ChaosEventType.Waiting},
                };

        public static NativeTypes.FABRIC_CHAOS_EVENT_KIND GetNativeEventType(ChaosEvent e)
        {
            if (!ChaosEventTypeToIntMap.ContainsKey(e.GetType()))
            {
                throw new InvalidOperationException("Unknown Chaos event type.");
            }
            else
            {
                return (NativeTypes.FABRIC_CHAOS_EVENT_KIND)ChaosEventTypeToIntMap[e.GetType()];
            }
        }

        public static ChaosEvent FromNativeEvent(NativeTypes.FABRIC_CHAOS_EVENT nativeEvent)
        {
            switch (nativeEvent.Kind)
            {
                case NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_STARTED:
                    return StartedEvent.FromNative(nativeEvent.Value);
                case NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_EXECUTING_FAULTS:
                    return ExecutingFaultsEvent.FromNative(nativeEvent.Value);
                case NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_STOPPED:
                    return StoppedEvent.FromNative(nativeEvent.Value);
                case NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_TEST_ERROR:
                    return TestErrorEvent.FromNative(nativeEvent.Value);
                case NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_VALIDATION_FAILED:
                    return ValidationFailedEvent.FromNative(nativeEvent.Value);
                case NativeTypes.FABRIC_CHAOS_EVENT_KIND.FABRIC_CHAOS_EVENT_KIND_WAITING:
                    return WaitingEvent.FromNative(nativeEvent.Value);
                default:
                    return null;
            }
        }

        /// <summary>
        /// If it is a native or rest call, reason in TestErrorEvent, ValidationFailedEvent, or WaitingEvent
        /// is not compressed, but the reason could be a valid base64 string by itself,
        /// thus the reason being not compressed will not be caught in Decompress.
        /// This method makes the reason not a base64 when it is not compressed (i.e., it was rest or native client).
        /// </summary>
        /// <param name="reason">Reason in TestErrorEvent, ValidationFailedEvent, and WaitingEvent</param>
        /// <returns>If reason has length a multiple of 4, this returns the floor or ceil that is not a multiple of 4.</returns>
        public static string MakeLengthNotMultipleOfFour(string reason)
        {
            if (reason.Length % 4 == 0)
            {
                if (reason.Length == ChaosConstants.StringLengthLimit)
                {
                    return reason.Substring(0, reason.Length-1);
                }
                else
                {
                    return string.Format("{0}.", reason);
                }
            }

            return reason;
        }

        /// <summary>
        /// If it is a native or rest call, reason in TestErrorEvent, ValidationFailedEvent, or WaitingEvent
        /// is not compressed, but the reason could be a valid base64 string by itself,
        /// thus the reason being not compressed will not be caught in Decompress.
        /// This method makes the reason not a base64 when it is not compressed (i.e., it was rest or native client).
        /// </summary>
        /// <param name="reason">Reason in TestErrorEvent, ValidationFailedEvent, and WaitingEvent</param>
        /// <returns>If reason has length a multiple of 4, this returns the floor or ceil that is not a multiple of 4.</returns>
        public static string MakeLengthNotMultipleOfFourIgnoreReasonLength(string reason)
        {
            if (reason.Length % 4 == 0)
            {
                return string.Format("{0}.", reason);
            }

            return reason;
        }

        public static string Compress(string text)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "compressing the text: {0}", text);

            byte[] buffer = Encoding.UTF8.GetBytes(text);

            using (MemoryStream ms = new MemoryStream())
            {
                using (GZipStream zip = new GZipStream(ms, CompressionMode.Compress, true))
                {
                    zip.Write(buffer, 0, buffer.Length);
                }

                ms.Position = 0;

                byte[] compressed = new byte[ms.Length];
                ms.Read(compressed, 0, compressed.Length);

                byte[] gzBuffer = new byte[compressed.Length + 4];
                Buffer.BlockCopy(compressed, 0, gzBuffer, 4, compressed.Length);
                Buffer.BlockCopy(BitConverter.GetBytes(buffer.Length), 0, gzBuffer, 0, 4);
                return Convert.ToBase64String(gzBuffer);
            }
        }

        public static string Decompress(string compressedText)
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "decompressing the compressedText: {0}", compressedText);

            byte[] gzBuffer = null;

            try
            {
                gzBuffer = Convert.FromBase64String(compressedText);
            }
            catch(FormatException)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Returning the input as it is, because decompress failed for input: '{0}'", compressedText);
                return compressedText;
            }

            using (MemoryStream ms = new MemoryStream())
            {
                int msgLength = BitConverter.ToInt32(gzBuffer, 0);
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Decompress msg length of compressed text is: {0}", msgLength);

                ms.Write(gzBuffer, 4, gzBuffer.Length - 4);

                byte[] buffer = new byte[msgLength];

                ms.Position = 0;
                using (GZipStream zip = new GZipStream(ms, CompressionMode.Decompress))
                {
                    zip.Read(buffer, 0, buffer.Length);
                }

                return Encoding.UTF8.GetString(buffer);
            }
        }

        /// <summary>
        /// In test this asserts and in production this throws
        /// </summary>
        /// <param name="id">Needed to do separate aggregation for different kinds of ThrowOrAssert</param>
        /// <param name="condition"></param>
        /// <param name="message"></param>
        /// <param name="snapshot"></param>
        public static void ThrowOrAssertIfTrue(string id, bool condition, string message, ClusterStateSnapshot snapshot = null)
        {
            // 'ForceEngineAssertEnabled' is used by Test to produce TestErrorEvent at whim
            if (condition || ForcedEngineAssertEnabled)
            {
                FabricEvents.Events.ChaosEngineError(id, message);

                if (snapshot != null)
                {
                    snapshot.WriteToTrace(customPreamble: ChaosConstants.PreAssertAmble, customPostamble: ChaosConstants.PostAssertAmble);
                }

                if (EngineAssertEnabled)
                {
                    ReleaseAssert.Failfast(message);
                }
                else
                {
                    throw new FabricChaosEngineException(message);
                }
            }
        }

        public static void SetPublicProperty(this object destinationObject, PropertyInfo destinationProperty, object src)
        {
            if (src == null)
            {
                return;
            }

            destinationProperty?.SetValue(destinationObject, src.GetValueOfPublicProperty(destinationProperty.Name), null);
        }

        public static object GetValueOfPublicProperty(this object src, string propertyName)
        {
            return src.GetType().GetProperty(propertyName, BindingFlags.Public | BindingFlags.Instance)?.GetValue(src, null);
        }

        /// <summary>
        /// Recursively visits through the properties of the object and based on mode takes appropriate action.
        /// </summary>
        /// <param name="obj">Object to visit</param>
        /// <param name="name">Fully qualified name of the property to take action on. We insist on qualifying the property fully like 'System.Fabric.Chaos.DataStructures.ChaosParameters.MaxConcurrentFaults' so that in the property graph we know exactly which property we are targetting.</param>
        /// <param name="value">Value of the target property</param>
        /// <param name="mode">If 'Update' updates the target property with 'value', if 'Verify' checks if the target property has 'value'</param>
        public static void VisitObject(object obj, string name, string value, ObjectVisitMode mode)
        {
            if (string.IsNullOrEmpty(name) || string.IsNullOrEmpty(value))
            {
                return;
            }

            Helper(obj, name, value, mode, new HashSet<string>(StringComparer.Ordinal));
        }

        public static NativeTypes.NativeFILETIME ToNativeFILETIMENormalTimeStamp(DateTime dateTime)
        {
            Int64 fileTimeUtc = dateTime.ToFileTimeUtc();

            NativeTypes.NativeFILETIME nativeFileTime = new NativeTypes.NativeFILETIME();
            nativeFileTime.dwLowDateTime = (UInt32)(fileTimeUtc & 0xFFFFFFFF);
            nativeFileTime.dwHighDateTime = (UInt32)(fileTimeUtc >> 32);

            return nativeFileTime;
        }

        /// <summary>
        /// Recursively visits through the properties of the object and based on mode takes appropriate action.
        /// </summary>
        /// <param name="obj">Object to visit</param>
        /// <param name="name">Fully qualified name of the property to take action on. We insist on qualifying the property fully like 'System.Fabric.Chaos.DataStructures.ChaosParameters.MaxConcurrentFaults' so that in the property graph we know exactly which property we are targetting.</param>
        /// <param name="value">Value of the target property</param>
        /// <param name="mode">If 'Update' updates the target property with 'value', if 'Verify' checks if the target property has 'value'</param>
        /// <param name="visited">To avoid cycles, contains the names of the properties that have already been visited and if encountered again avoids digging deeper.</param>
        private static void Helper(object obj, string name, string value, ObjectVisitMode mode, HashSet<string> visited)
        {
            if (obj == null)
            {
                return;
            }

            Type objType = obj.GetType();
            PropertyInfo[] properties = objType.GetProperties(BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public);

            object propValue = null;

            // Check if any property of obj matches the provided 'name'
            foreach (PropertyInfo property in properties)
            {
                string propertyFullName = objType.FullName + "." + property.Name;

                if (visited.Contains(propertyFullName))
                {
                    continue;
                }

                if ( MatchSuffix(propertyFullName, name) )
                {
                    // Handle list type property
                    if (property.PropertyType.IsGenericType && property.PropertyType.GetGenericTypeDefinition() == typeof(List<>))
                    {
                        Type listItemType = property.PropertyType.GetGenericArguments()[0];
                        if (listItemType == null)
                        {
                            return;
                        }

                        var resultList = CreateList(listItemType);

                        var inputlist = GetList(listItemType, value, ListElementSeparator);

                        switch (mode)
                        {
                            case ObjectVisitMode.Update:

                                foreach (var item in inputlist)
                                {
                                    resultList.Add(item);
                                }

                                property.SetValue(obj, resultList, null);
                                break;

                            case ObjectVisitMode.Verify:

                                IList propertyValue = property.GetValue(obj, null) as IList;

                                if (propertyValue == null || propertyValue.Count != inputlist.Count)
                                {
                                    throw new InvalidOperationException("propertyValue should have been a list of the same size as value.");
                                }

                                for (int i = 0; i < inputlist.Count; ++i)
                                {
                                    if (!inputlist[i].Equals(propertyValue[i]))
                                    {
                                        throw new InvalidOperationException("property does not have the expected value.");
                                    }
                                }

                                break;
                        }
                    }
                    else if (property.PropertyType.GetInterface(IEnumerableInterfaceName) == null) // not a collection type
                    {
                        object inputValue = Convert.ChangeType(value, property.PropertyType);
                        propValue = property.GetValue(obj, null);

                        switch (mode)
                        {
                            case ObjectVisitMode.Update:

                                property.SetValue(obj, inputValue, null);
                                break;

                            case ObjectVisitMode.Verify:

                                if (!propValue.Equals(inputValue))
                                {
                                    throw new InvalidOperationException("property does not have the expected value.");
                                }

                                break;
                        }
                    }

                    return;
                }

                try
                {
                    visited.Add(propertyFullName);

                    propValue = property.GetValue(obj, null);
                    if (property.PropertyType.Assembly == objType.Assembly)
                    {
                        Helper(propValue, name, value, mode, visited);
                    }
                }
                catch (TargetParameterCountException)
                {
                    if (!property.GetIndexParameters().Any())
                    {
                        throw;
                    }
                }
            }
        }

        /// <summary>
        /// Matches progressively longer suffixes of haystack with needle.
        /// Note, assumes both haystack and needle to be non-empty.
        /// </summary>
        /// <param name="haystack">In the string where needle should be searched for.</param>
        /// <param name="needle">The string that we are looking for.</param>
        /// <returns>True if any suffix of haystack matches needle, otherwise False.</returns>
        private static bool MatchSuffix(string haystack, string needle)
        {
            string[] haystackParts = haystack.Split(NamespaceSeparator);
            string[] needleParts = needle.Split(NamespaceSeparator);

            int i = haystackParts.Length - 1, j = needleParts.Length - 1;
            while (i >= 0 && j >= 0)
            {
                if (!haystackParts[i--].Equals(needleParts[j--], StringComparison.Ordinal))
                {
                    return false;
                }
            }

            return (j < 0);
        }

        /// <summary>
        /// Splits value based on separator and returns as a list.
        /// </summary>
        /// <param name="T">The type of the list elements.</param>
        /// <param name="value">The string to parse to get the string elements.</param>
        /// <param name="separator">Delimeter to split the string.</param>
        /// <returns>List of the separated parts.</returns>
        private static IList GetList(Type T, string value, char separator)
        {
            IList result = CreateList(T);

            if (string.IsNullOrEmpty(value))
            {
                return result;
            }

            foreach (string item in value.Split(separator).ToList())
            {
                result.Add(Convert.ChangeType(item, T));
            }

            return result;
        }

        /// <summary>
        /// Initializes a list of type type
        /// </summary>
        /// <param name="type">Element type</param>
        /// <returns>A list of type</returns>
        private static IList CreateList(Type type)
        {
            Type genericListType = typeof(List<>).MakeGenericType(type);
            return (IList)Activator.CreateInstance(genericListType);
        }
    }
}