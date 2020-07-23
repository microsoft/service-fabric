// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Controllers
{
    using System;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.Net;
    using System.Net.Http;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Web.Http;
    using EventStore.Service.ApiEndpoints.Operations;
    using EventStore.Service.ApiEndpoints.Server;
    using EventStore.Service.Exceptions;
    using EventStore.Service.Types;
    using LogProvider;

    public class BaseController : ApiController
    {
        public BaseController(ControllerSetting controllerSetting)
        {
            Assert.IsNotNull(controllerSetting, "controllerSetting != null");
            Assert.IsNotNull(controllerSetting.CurrentRuntime, "controllerSetting.CurrentRuntime != null");
            this.CurrentRuntime = controllerSetting.CurrentRuntime;
        }

        protected EventStoreRuntime CurrentRuntime { get; private set; }

        internal async Task<TResult> RunAsync<TResult>(BaseOperation<TResult> operationBase, int timeoutInSeconds = (2 * 60))
        {
            var cancellationTokenSource = new CancellationTokenSource();
            if (timeoutInSeconds <= 0)
            {
                throw new ArgumentException("Invalid Timeout");
            }

            var timeout = new TimeSpan(0, 0, timeoutInSeconds);
            cancellationTokenSource.CancelAfter(timeout);

            await this.ValidateInputAsync();

            try
            {
                var startTime = DateTimeOffset.UtcNow;

                var results = await operationBase.RunAsyncWrappper(timeout, cancellationTokenSource.Token).ConfigureAwait(false);

                return results;
            }
            catch (Exception exception)
            {
                FabricEvents.Events.EventStoreFailed(operationBase.GetSupportedEntityType().ToString(), ExtractDetails(exception));

                if (exception is ConnectionParsingException)
                {
                    // This indicates to the caller that the API's are not available as the connection information is not configured.
                    throw new HttpResponseException(HttpStatusCode.ServiceUnavailable);
                }

                if (exception is HttpResponseException)
                {
                    throw;
                }

                var fabricException = exception as FabricException;
                if (fabricException != null)
                {
                    var fabricError = new FabricError(fabricException);
                    var fabricBackupRestoreError = new FabricErrorError(fabricError);
                    throw new HttpResponseException(fabricBackupRestoreError.ToHttpResponseMessage());
                }
                else if (exception is ArgumentException)
                {
                    FabricErrorError fabricEventStoreSvcErrorMessage = new FabricErrorError(
                        new FabricError
                        {
                            Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                            Message = exception.Message
                        });
                    throw new HttpResponseException(fabricEventStoreSvcErrorMessage.ToHttpResponseMessage());
                }
                else if (exception is OperationCanceledException || exception is TimeoutException)
                {
                    FabricErrorError fabricEventStoreSvcErrorMessage = new FabricErrorError(
                        new FabricError
                        {
                            Code = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT,
                            Message = "Timeout"
                        });
                    throw new HttpResponseException(fabricEventStoreSvcErrorMessage.ToHttpResponseMessage());
                }
                else
                {
                    var stringBuilder = new StringBuilder();
                    if (exception is AggregateException)
                    {
                        var aggregateException = (AggregateException)exception;
                        foreach (var innerException in aggregateException.InnerExceptions)
                        {
                            stringBuilder.AppendFormat("{0} ,", innerException.Message).AppendLine();
                        }
                    }
                    else
                    {
                        stringBuilder.AppendFormat("{0}", exception.Message);
                    }
                    FabricErrorError fabricEventStoreSvcErrorMessage = new FabricErrorError(
                        new FabricError
                        {
                            Message = stringBuilder.ToString(),
                            Code = NativeTypes.FABRIC_ERROR_CODE.E_UNEXPECTED
                        });
                    throw new HttpResponseException(fabricEventStoreSvcErrorMessage.ToHttpResponseMessage());
                }
            }

            throw new HttpResponseException(HttpStatusCode.InternalServerError);
        }

        private async Task ValidateInputAsync()
        {
            if (this.ModelState.IsValid)
            {
                return;
            }

            this.LogErrorInModels();

            var badRequestMessage = this.Request.CreateErrorResponse(HttpStatusCode.BadRequest, this.ModelState);

            var badRequestMessageDetails = await badRequestMessage.Content.ReadAsStringAsync();

            var eventStoreInputValidationError = new FabricErrorError(
                new FabricError
                {
                    Message = badRequestMessageDetails.Replace(@"""", ""),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG
                });

            throw new HttpResponseException(eventStoreInputValidationError.ToHttpResponseMessage());
        }

        private void LogErrorInModels()
        {
            var errorStringBuilder = new StringBuilder("Validation Failed for the following errors:").AppendLine();
            foreach (var errorKey in this.ModelState.Keys)
            {
                errorStringBuilder.AppendFormat("Errors for the Key {0} : ", errorKey).AppendLine();
                var modelStateError = this.ModelState[errorKey];
                errorStringBuilder.AppendFormat("Model State Errors Are : ").AppendLine();
                foreach (var modelError in modelStateError.Errors)
                {
                    errorStringBuilder.AppendFormat("Exception : {0} , ErrorMessage : {1} ", modelError.Exception,
                        modelError.ErrorMessage)
                        .AppendLine();
                }
                if (modelStateError.Value != null)
                {
                    errorStringBuilder.AppendFormat("Value Provider Result are : ").AppendLine();
                    errorStringBuilder.AppendFormat(
                        "Attempted Value = {0} , Raw Values = {1} , Culture Info DisplayName = {2}",
                        modelStateError.Value.AttemptedValue, modelStateError.Value.RawValue,
                        modelStateError.Value.Culture.DisplayName).AppendLine();
                }
            }
            EventStoreLogger.Logger.LogWarning("Validation Failed for Request {0} with Errors : {1} ",
               this.GetType(), errorStringBuilder.ToString());
        }

        private static string ExtractDetails(Exception exception)
        {
            var errorStringBuilder = new StringBuilder("Encountered following Exception:").AppendLine();
            if (exception is AggregateException)
            {
                errorStringBuilder.AppendFormat("Controller : Aggregate Exception Stack Trace : {0}", exception.StackTrace);
                var aggregateException = (AggregateException)exception;
                foreach (var innerException in aggregateException.InnerExceptions)
                {
                    errorStringBuilder.AppendFormat(
                       "Controller: Inner Exception Message : {0} , Stack Trace : {1} ",
                       innerException.Message == null ? "Empty" : innerException.Message,
                       innerException.StackTrace);
                }
            }
            else
            {
                errorStringBuilder.AppendFormat(
                    "Controller : Exception Message : {0} , Stack Trace : {1} ",
                    exception.Message == null ? "Empty" : exception.Message,
                    exception.StackTrace);
            }

            return errorStringBuilder.ToString();
        }
    }
}