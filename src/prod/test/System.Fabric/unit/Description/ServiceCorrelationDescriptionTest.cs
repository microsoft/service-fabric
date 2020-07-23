// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceCorrelationDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<ServiceCorrelationDescription, object>
        {
            public override void Compare(ServiceCorrelationDescription expected, ServiceCorrelationDescription actual)
            {
                Assert.AreEqual<ServiceCorrelationScheme>(expected.Scheme, actual.Scheme);
                Assert.AreEqual<Uri>(expected.ServiceName, actual.ServiceName);
            }

            public override ServiceCorrelationDescription Factory(object info)
            {
                throw new NotImplementedException();
            }

            public override object CreateDefaultInfo()
            {
                throw new NotImplementedException();
            }

            public override ServiceCorrelationDescription CreateDefaultDescription()
            {
                return new ServiceCorrelationDescription
                {
                    Scheme = ServiceCorrelationScheme.Affinity,
                    ServiceName = new Uri("fabric:/myservice"),
                };
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServiceCorrelationDescription_Parse()
        {
            var expected = HelperInstance.CreateDefaultDescription();

            using (var pc = new PinCollection())
            {
                NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION[] native = new NativeTypes.FABRIC_SERVICE_CORRELATION_DESCRIPTION[1];
                expected.ToNative(pc, ref native[0]);

                var actual = ServiceCorrelationDescription.CreateFromNative(pc.AddBlittable(native));

                HelperInstance.Compare(expected, actual);
            }
        }
    }
}