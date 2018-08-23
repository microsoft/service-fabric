// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Globalization;

    internal sealed class TestCommandListDescription
    {
        public TestCommandListDescription(
            TestCommandStateFilter commandStateFilter,
            TestCommandTypeFilter commandTypeFilter)
        {
            this.TestCommandStateFilter = commandStateFilter;
            this.TestCommandTypeFilter = commandTypeFilter;
        }

        public TestCommandStateFilter TestCommandStateFilter
        {
            get;
            internal set;
        }

        public TestCommandTypeFilter TestCommandTypeFilter
        {
            get;
            internal set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "TestCommandStateFilter: {0}, TestCommandTypeFilter: {1}",
                this.TestCommandStateFilter,
                this.TestCommandTypeFilter);
        }

        // Used on client side
        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_TEST_COMMAND_LIST_DESCRIPTION();

            nativeDescription.TestCommandStateFilter = (UInt32)this.TestCommandStateFilter;
            nativeDescription.TestCommandTypeFilter = (UInt32)this.TestCommandTypeFilter;

            return pinCollection.AddBlittable(nativeDescription);
        }

        // Used on service side
        internal static unsafe TestCommandListDescription CreateFromNative(IntPtr nativePtr)
        {
            NativeTypes.FABRIC_TEST_COMMAND_LIST_DESCRIPTION nativeDescription = *(NativeTypes.FABRIC_TEST_COMMAND_LIST_DESCRIPTION*)nativePtr;

            TestCommandStateFilter stateFilter = (TestCommandStateFilter)nativeDescription.TestCommandStateFilter;
            TestCommandTypeFilter typeFilter = (TestCommandTypeFilter)nativeDescription.TestCommandTypeFilter;

            return new TestCommandListDescription(stateFilter, typeFilter);
        }
    }
}
