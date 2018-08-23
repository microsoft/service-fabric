// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.Fabric.Strings;
    using System.Runtime.Serialization;

    [Serializable]
    internal class FabricImageStoreIOException : Exception
    {
        public FabricImageStoreIOException()
        {
        }

        public FabricImageStoreIOException(string message)
            : base(message)
        {
        }

        public FabricImageStoreIOException(Exception inner)
            : this(StringResources.ImageStoreError_IOException, inner)
        {
        }

        public FabricImageStoreIOException(string message, Exception inner)
            : base(message, inner)
        {
        }

        protected FabricImageStoreIOException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
    }
}