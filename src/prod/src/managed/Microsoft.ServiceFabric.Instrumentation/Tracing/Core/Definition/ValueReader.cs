// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Net;
    using System.Runtime.Serialization;

    /// <summary>
    /// Contract for value Reader
    /// </summary>
    /// <remarks>
    /// Let's dig a little deeper into the purpose of this interface. If you look at the definitions we have defined,
    /// they define the structure of an expected Trace that the product is generating. When we are reading the actual traces
    /// from the source, the read values still need to be used to populate this structure. The way we will read the value and use
    /// them to populate the actual objects will differ based on the source of the traces. For e.g., if we are reading from ETW file,
    /// the data is in raw bytes, and we need to parse it to get to actual values that can be used to create the appropriate TraceRecord
    /// Object. Similarly, when we are reading from Azure table, we will have to do some property bag parsing. Ok, so I'm assuming that I have
    /// established the need to do parsing and how this parsing will vary from source to source.
    /// The next thing to note is that we are making an assumption that most of the traces will be read and not all values will be read. That is
    /// usually the case. So, the model we have picked is that instead of directly populating each trace object fully, we will attach a "ValueReader"
    /// to the object. This value reader will read the actual value when the property is accessed. If you look at the implementation of "ValueReader",
    /// it's pretty straightfoward, and quite efficient. It's literally the same logic, but its execution is delayed from the point of object creation,
    /// to the point when the specific property is read. It does add a tiny, tiny bit of extra compute, but that is more than offset by the fact that not 
    /// all values will be read.
    /// We are making it an abstract class so that we can do serialization, something that's not an option for interfaces.
    /// </remarks>
    [DataContract]
    [KnownType("GetKnownTypes")]
    public abstract class ValueReader
    {
        private static List<Type> knownTypes;

        /// <summary>
        /// Given an Offset to a null terminated ASCII string in an event blob, return the string that is
        /// held there.   
        /// </summary>
        public abstract string ReadUtf8StringAt(int index);

        /// <summary>
        /// Returns the encoding of a Version 6 IP address that has been serialized at 'offset' in the payload bytes.  
        /// </summary>
        public abstract IPAddress ReadIpAddrV6At(int index);

        /// <summary>
        /// Returns the GUID serialized at 'offset' in the payload bytes. 
        /// </summary>
        public abstract Guid ReadGuidAt(int index);

        /// <summary>
        /// Get the DateTime that serialized (as a windows FILETIME) at 'offset' in the payload bytes. 
        /// </summary>
        /// <remarks>
        /// We always assume that time read is in UTC timezone and mark the returned object as UTC.
        /// </remarks>
        public abstract DateTime ReadDateTimeAt(int index);

        /// <summary>
        /// Given an Offset to a null terminated Unicode string in an payload bytes, return the string that is
        /// held there.   
        /// </summary>
        public abstract string ReadUnicodeStringAt(int index);

        /// <summary>
        /// Give an offset to a byte array of size 'size' in the payload bytes, return a byte[] that contains
        /// those bytes.
        /// </summary>
        public abstract byte[] ReadByteArrayAt(int index, int size);

        /// <summary>
        /// Returns a byte value that was serialized at 'index' in the payload bytes
        /// </summary>
        public abstract byte ReadByteAt(int index);

        /// <summary>
        /// Returns a short value that was serialized at 'index' in the payload bytes
        /// </summary>
        public abstract short ReadInt16At(int index);

        /// <summary>
        /// Returns an int value that was serialized at 'index' in the payload bytes
        /// </summary>
        public abstract int ReadInt32At(int index);

        /// <summary>
        /// Returns a long value that was serialized at 'index' in the payload bytes
        /// </summary>
        public abstract long ReadInt64At(int index);

        /// <summary>
        /// Returns a bool value from this index.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        public abstract bool ReadBoolAt(int index);

        /// <summary>
        /// Returns an int float (single) that was serialized at 'index' in the payload bytes
        /// </summary>
        public abstract float ReadSingleAt(int index);

        /// <summary>
        /// Returns an int double precision floating point value that was serialized at 'index' in the payload bytes
        /// </summary>
        public abstract double ReadDoubleAt(int index);

        /// <summary>
        /// Clone Object
        /// </summary>
        /// <returns></returns>
        public abstract ValueReader Clone();

        internal static IList<Type> GetKnownTypes()
        {
            if (knownTypes != null)
            {
                return knownTypes;
            }

            knownTypes = new List<Type> { typeof(ValueReader) };
            var assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (var assembly in assemblies)
            {
                try
                {
                    var types = assembly.GetTypes();
                    knownTypes.AddRange(types.Where(type => type.IsSubclassOf(typeof(ValueReader))));
                }
                catch (Exception)
                {
                    // ignored
                }
            }

            return knownTypes;
        }
    }
}