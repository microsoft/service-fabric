// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Fabric.ReliableMessaging;
    using System.IO;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters.Binary;

    #endregion

    /// <summary>
    /// Static Generics Serializers
    /// </summary>
    public static class ByteArraySerializer
    {
        /// <summary>
        /// Serialize an given T object.
        /// This is less efficient than custom serializers, use only for infrequent use when performance is not a constraint.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="t"></param>
        /// <returns></returns>
        public static byte[] Serialize<T>(this T t)
        {
            Diagnostics.Assert(t.GetType().GetTypeInfo().IsSerializable, "Object is not Serializable");

            using (var stream = new MemoryStream())
            {
                new BinaryFormatter().Serialize(stream, t);
                return stream.ToArray();
            }
        }

        /// <summary>
        /// DeSerialize to a T object.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="byteArray"></param>
        /// <returns></returns>
        public static T Deserialize<T>(this byte[] byteArray)
        {
            using (var stream = new MemoryStream(byteArray))
            {
                return (T) new BinaryFormatter().Deserialize(stream);
            }
        }
    }
}