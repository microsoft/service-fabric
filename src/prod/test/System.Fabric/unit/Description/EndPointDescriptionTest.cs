// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Description
{
    using System.Fabric.Description;
    using System.Fabric.Test.Stubs;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class EndpointDescriptionTest
    {
        internal class Helper : DescriptionElementTestBase<EndpointResourceDescription, EndPointInfo>
        {

            public override EndpointResourceDescription Factory(EndPointInfo info)
            {
                return EndpointResourceDescription.CreateFromNative(TestUtility.StructureToIntPtr(info.ToNative()));
            }

            public override EndPointInfo CreateDefaultInfo()
            {
                return new EndPointInfo
                {
                    CertificateName = "Cert",
                    Name = "endpoint1",
                    Port = 231,
                    Protocol = "http",
                    Type = "Input",
                    UriScheme = "http",
                    PathSuffix = "/SomeApp"
                };
            }

            public override EndpointResourceDescription CreateDefaultDescription()
            {
                return new EndpointResourceDescription
                {
                    Certificate = "Cert",
                    EndpointType = EndpointType.Input,
                    Name = "endpoint1",
                    Protocol = EndpointProtocol.Http,
                    Port = 231,
                    UriScheme = "http",
                    PathSuffix = "/SomeApp"
                };
            }

            public override void Compare(EndpointResourceDescription expected, EndpointResourceDescription actual)
            {
                Assert.AreEqual(expected.Certificate, actual.Certificate);
                Assert.AreEqual(expected.EndpointType, actual.EndpointType);
                Assert.AreEqual(expected.Name, actual.Name);
                Assert.AreEqual(expected.Protocol, actual.Protocol);
                Assert.AreEqual(expected.Port, actual.Port);
                Assert.AreEqual(expected.UriScheme, actual.UriScheme);
                Assert.AreEqual(expected.PathSuffix, actual.PathSuffix);
            }
        }

        internal static Helper HelperInstance
        {
            get { return new Helper(); }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndpointDescription_BasicParse()
        {
            HelperInstance.ParseTestHelper(null, null);
        }

        private void EndPointTestHelper(string nativeType, EndpointType managedType)
        {
            HelperInstance.ParseTestHelper((info) => info.Type = nativeType, (description) => description.EndpointType = managedType);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndpointDescription_EndpointType()
        {
            this.EndPointTestHelper("Input", EndpointType.Input);
            this.EndPointTestHelper("Internal", EndpointType.Internal);
        }

        private void EndpointProtocolTestHelper(string nativeProtocol, string uriScheme, EndpointProtocol protocol)
        {
            HelperInstance.ParseTestHelper(
                (info) => info.Protocol = nativeProtocol,
                (description) => description.Protocol = protocol);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndpointDescription_EndpointProtocol()
        {
            this.EndpointProtocolTestHelper("http", "http", EndpointProtocol.Http);
            this.EndpointProtocolTestHelper("https", "https", EndpointProtocol.Https);
            this.EndpointProtocolTestHelper("tcp", "net.tcp", EndpointProtocol.Tcp);
        }
    }
}