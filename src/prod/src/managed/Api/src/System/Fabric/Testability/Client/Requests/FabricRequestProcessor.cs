// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Linq;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Testability.Client.Structures;
    using System.Runtime.InteropServices;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Fabric.Query;
    using System.Collections.Generic;

    public static class FabricRequestProcessor
    {
        public static Predicate<IRetryContext> SuccessRetryErrorCodeProcessor(FabricRequest request)
        {
            return delegate(IRetryContext r)
            {
                if (request.OperationResult != null)
                {
                    uint errorCode = request.OperationResult.ErrorCode;
                    if (!request.SucceedErrorCodes.Contains(errorCode))
                    {
                        if (request.RetryErrorCodes.Contains(errorCode))
                        {
                            r.ShouldRetry = true;
                            r.Succeeded = false;

                            TestabilityTrace.TraceSource.WriteInfo("FabricRequestProcessor", "{0}: Request failed with '{1}'. Retry is required.", r.ActivityId, errorCode);
                        }
                        else
                        {
                            r.ShouldRetry = false;
                            r.Succeeded = false;

                            TestabilityTrace.TraceSource.WriteWarning("FabricRequestProcessor", "{0}: Request failed with '{1}'. The error code is not in success and retry error codes.", r.ActivityId, errorCode);
                            
                            string errorMessage = StringHelper.Format(
                                "{0} failed with {1}. The error code is not in success & retry error codes.",
                                request.ToString(),
                                errorCode);

                            Func<Exception, string, Exception> translateExceptionFunction = null;
                            if (!System.Fabric.Interop.InteropExceptionMap.NativeToManagedConversion.TryGetValue((int)request.OperationResult.ErrorCode, out translateExceptionFunction))
                            {
                                TestabilityTrace.TraceSource.WriteError(
                                    "FabricRequestProcessor",
                                    "Unknown ErrorCode {0} which cannot be mapped to any Fabric related exception",
                                    request.OperationResult.ErrorCode);

                                throw new System.Fabric.FabricException(StringResources.Error_Unknown, FabricErrorCode.Unknown);
                            }

                            // Creates an inner com exception with the native error code as well and error message. 
                            // This can be improved by somehow getting the original exception thrown which was handled by the code
                            Exception exceptionToThrow = translateExceptionFunction(new COMException(errorMessage, (int)request.OperationResult.ErrorCode), errorMessage);
                            throw exceptionToThrow;
                        }
                    }
                    else
                    {
                        r.ShouldRetry = false;
                        r.Succeeded = true;

                        TestabilityTrace.TraceSource.WriteNoise("FabricRequestProcessor", "{0}: Request succeeded with '{1}'.", r.ActivityId, errorCode);
                    }
                }
                else
                {
                    r.ShouldRetry = false;
                    r.Succeeded = false;
                    throw new InvalidOperationException(StringHelper.Format("Operation result is null for '{0}'", request.ToString()));
                }

                return r.Succeeded;
            };
        }

        public static Predicate<IRetryContext> ExpectedInstanceCount(FabricRequest request, int expectedInstanceCount)
        {
            return FabricRequestProcessor.ExpectedResultCount<TestServicePartitionInfo>(request, expectedInstanceCount);
        }

        public static Predicate<IRetryContext> ExpectedPartitionCount(FabricRequest request, int expectedResultCount)
        {
            return FabricRequestProcessor.ResultCountConstraint<ServicePartitionList, Partition>(request, expectedResultCount, (a, b) => a >= b);
        }

        public static Predicate<IRetryContext> ExpectedResultCount<TList, TItem>(FabricRequest request, int expectedResultCount) where TList : IList<TItem>
        {
            return FabricRequestProcessor.ResultCountConstraint<TList, TItem>(request, expectedResultCount, (a, b) => a == b);
        }

        public static Predicate<IRetryContext> ExpectedResultCount<T>(FabricRequest request, int expectedResultCount)
        {
            return FabricRequestProcessor.ResultCountConstraint<T>(request, expectedResultCount, (a, b) => a == b);
        }

        public static Predicate<IRetryContext> MinimumExpectedResultCount<T>(FabricRequest request, int expectedResultCount)
        {
            return FabricRequestProcessor.ResultCountConstraint<T>(request, expectedResultCount, (a, b) => a >= b);
        }

        private static Predicate<IRetryContext> ResultCountConstraint<T>(FabricRequest request, int expectedResultCount, Func<int, int, bool> condition)
        {
            return ResultCountConstraint<T[], T>(request, expectedResultCount, condition);
        }

        private static Predicate<IRetryContext> ResultCountConstraint<TList, TItem>(FabricRequest request, int expectedResultCount, Func<int, int, bool> condition) where TList : IList<TItem>
        {
            ThrowIf.OutOfRange(expectedResultCount, 0, int.MaxValue, "expectedResultCount");

            return delegate(IRetryContext r)
            {
                Predicate<IRetryContext> errorCodeChecker = FabricRequestProcessor.SuccessRetryErrorCodeProcessor(request);

                // Only check the instance count if the request was a success AND the error code is S_OK. Some resolve requests
                // are used to check if the service has been deleted properly and will not have an S_OK success associated with them...
                if (errorCodeChecker(r) && request.OperationResult.ErrorCode == 0)
                {
                    OperationResult<TList> actualResult = (OperationResult<TList>)request.OperationResult;
                    int actualResultLength = actualResult.Result.Count();

                    if (condition(actualResultLength, expectedResultCount))
                    {
                        r.ShouldRetry = false;
                        r.Succeeded = true;

                        TestabilityTrace.TraceSource.WriteNoise("FabricRequestProcessor", "{0}: Request succeeded with expected result count: '{1}'", r.ActivityId, actualResultLength);
                    }
                    else
                    {
                        ResolveServiceRequest tempRequest = request as ResolveServiceRequest;
                        if (tempRequest != null && typeof(TItem) == typeof(TestServicePartitionInfo))
                        {
                            tempRequest.PreviousResult = actualResult.Result.First() as TestServicePartitionInfo;
                        }

                        r.ShouldRetry = true;
                        r.Succeeded = false;

                        TestabilityTrace.TraceSource.WriteInfo(
                            "FabricRequestProcessor",
                            "{0}: Request failed in result count validation. Expected result count: '{1}'. Actual result count : '{2}'",
                            r.ActivityId,
                            expectedResultCount,
                            actualResultLength);
                    }
                }

                return r.Succeeded;
            };
        }
    }
}

#pragma warning restore 1591