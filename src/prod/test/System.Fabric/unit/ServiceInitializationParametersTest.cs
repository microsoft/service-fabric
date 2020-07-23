// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Fabric;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceInitializationParametersTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceInitializationParameters_NullActivationContextThrowsInvalidOp()
        {
            Action<ServiceInitializationParameters> action = (initParams) =>
            {
                TestUtility.ExpectException<InvalidOperationException>(() =>
                {
                    var t = initParams.CodePackageActivationContext;
                });
            };

            action(new StatelessServiceInitializationParameters(null));
            action(new StatefulServiceInitializationParameters(null));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void ServiceInitializationParameters_NonNullActivationContextReturnsObject()
        {
            var obj = new CodePackageActivationContext(new Stubs.CodePackageActivationContextStub());

            Action<ServiceInitializationParameters> action = (initParams) => Assert.AreSame(obj, initParams.CodePackageActivationContext);

            action(new StatelessServiceInitializationParameters(obj));
            action(new StatefulServiceInitializationParameters(obj));
        }
    }
}