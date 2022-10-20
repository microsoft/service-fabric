// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerWrapper
{
    using System.IO;

    public interface IJsonSerializer
    {
        void Serialize(object item, Stream stream);

        string Serialize(object obj);
        
        object Deserialize(string jsonString, Type type);

        object Deserialize(Stream stream, Type type);
    }
}