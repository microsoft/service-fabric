// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    internal sealed class CancelTestCommandDescription
    {
        public CancelTestCommandDescription(
            Guid operationId,
            bool force)
        {
            Requires.Argument<Guid>("operationId", operationId).NotNull();
            this.OperationId = operationId;
            this.Force = force;
        }

        public Guid OperationId
        {
            get;
            internal set;
        }

        public bool Force
        {
            get;
            internal set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "OperationId: {0}, Force: {1}",
                this.OperationId,
                this.Force);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION();

            nativeDescription.OperationId = this.OperationId;
            nativeDescription.Force = NativeTypes.ToBOOLEAN(this.Force);

            return pinCollection.AddBlittable(nativeDescription);
        }


        internal static unsafe CancelTestCommandDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION native = *(NativeTypes.FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION*)nativeRaw;

            Guid operationId = native.OperationId;            

            bool force = NativeTypes.FromBOOLEAN(native.Force);

            return new CancelTestCommandDescription(operationId, force);
        }

    }
}
