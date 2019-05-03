// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading;
    using Threading.Tasks;

    [TestClass]
    public class RetryPolicyTest
    {
        private static readonly TraceType TraceType = new TraceType("RetryPolicyTest");

        [TestMethod]
        public void ExecuteAsyncTest()
        {
            IRetryPolicy retryPolicy = new LinearRetryPolicyFactory(
                TraceType,
                500,
                3,
                IsRetriableException).Create();

            int retryCount = 0;

            try
            {
                retryPolicy
                    .ExecuteAsync(async (token) =>
                    {
                        retryCount++;

                        if (retryCount < 10)
                        {
                            throw new TimeoutException("timeout");
                        }

                        return await Task.FromResult(true);
                    },
                        "func1",
                        CancellationToken.None)
                    .GetAwaiter()
                    .GetResult();
            }
            catch (AggregateException ae)
            {
                AggregateException flatEx = ae.Flatten();

                flatEx.Handle(e => e is TimeoutException);

                Assert.AreEqual(retryCount, 3, "retry policy allowed retry 3 times as expected");
            }
        }

        private static bool IsRetriableException(Exception ex)
        {
            return ex is TimeoutException;
        }
    }
}