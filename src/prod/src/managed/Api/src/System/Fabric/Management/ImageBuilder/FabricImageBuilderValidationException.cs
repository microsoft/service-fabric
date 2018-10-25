// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Runtime.Serialization;

#if !DotNetCoreClr
    [Serializable]
#endif

    internal class FabricImageBuilderValidationException : FabricException
    {
        private string fileName;

        public FabricImageBuilderValidationException()
        {
        }

        public FabricImageBuilderValidationException(string message, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderValidationError)
            : this(message, string.Empty, errorCode)
        {            
        }

        public FabricImageBuilderValidationException(string message, string fileName, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderValidationError)
            : base(message, errorCode)
        {
            this.fileName = fileName;            
        }

        public FabricImageBuilderValidationException(string message, Exception inner, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderValidationError)
            : this(message, string.Empty, inner, errorCode)
        {
        }

        public FabricImageBuilderValidationException(string message, string fileName, Exception inner, FabricErrorCode errorCode = FabricErrorCode.ImageBuilderValidationError)
            : base(message, inner, errorCode)
        {
            this.fileName = fileName;            
        }

#if !DotNetCoreClr
        protected FabricImageBuilderValidationException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            this.fileName = info.GetString("FileName");
        }
#endif
        public override string Message
        {
            get
            {
                string message = base.Message;
                if (!string.IsNullOrEmpty(this.fileName))
                {
                    message = message + Environment.NewLine + string.Format(CultureInfo.InvariantCulture, "FileName: {0}", this.fileName);
                }

                return message;
            }
        }        
    }
}