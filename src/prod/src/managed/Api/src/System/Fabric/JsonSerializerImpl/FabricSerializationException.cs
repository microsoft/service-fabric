// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    internal class FabricSerializationException : FabricException
    {
        public FabricSerializationException() { }
        public FabricSerializationException(string message)
            :base(message)
        {
        }

        public FabricSerializationException(string message, Exception inner)
        : base(message, inner)
        {
        }
    }
}