// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.IO;

    /// <summary>
    /// <para>Exposes the methods to serialize the object into a byte[] or deserialize the object from a byte[]</para>
    /// </summary>
    public interface IByteSerializable
    {
        /// <summary>
        /// Retrieves a byte[] representation of the object
        /// </summary>
        /// <returns>A byte[]</returns>
        byte[] ToBytes();

        /// <summary>
        /// Populates an object from a byte[]
        /// </summary>
        /// <param name="data">byte[] representation of the object</param>
        void FromBytes(byte[] data);
    }
}