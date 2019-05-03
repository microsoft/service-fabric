// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers
{

    using System.Text;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations;
    using System.Threading.Tasks;
    using System.Web.Http;
    using System.Net;
    using System.Net.Http;
    using System.Fabric.Common;
    using System.Threading;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.BackupRestoreEndPoints;
    using System.Linq;
    using System.Fabric.Interop;

    public class BaseController : ApiController
    {
        private const string TraceType = "BaseController";
        private int _retryCount;
        private const int MaxRetryCount = 3;

        internal async Task<TResult> RunAsync<TResult>(BaseOperation<TResult> operationBase, int timeoutInSeconds)
        {
            var cancellationTokenSource = new CancellationTokenSource();
            if (timeoutInSeconds <= 0)
            {
                throw new ArgumentException(StringResources.InvalidTimeout);
            }

            var timeout = new TimeSpan(0, 0, timeoutInSeconds);
            cancellationTokenSource.CancelAfter(timeout);
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Controller {0} Operation requested : {1}", this.GetType(),operationBase);
            await this.ValidateInput();
            do
            {
                try
                {
                    this._retryCount++;
                    return await operationBase.RunAsyncWrappper(timeout, cancellationTokenSource.Token);
                }
                catch (HttpResponseException)
                {
                    throw;
                }
                catch (Exception exception)
                {
                    this.LogException(TraceType, exception);
                    var fabricException = exception as FabricException;
                    if (fabricException != null)
                    {
                        var fabricError = new FabricError(fabricException);
                        var fabricBackupRestoreError = new FabricErrorError(fabricError);
                        throw new HttpResponseException(fabricBackupRestoreError.ToHttpResponseMessage());
                    }else if (exception is ArgumentException)
                    {
                        FabricErrorError fabricBackupRestoreError = new FabricErrorError(new FabricError()
                        {
                            Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                            Message = exception.Message
                        });
                        throw new HttpResponseException(fabricBackupRestoreError.ToHttpResponseMessage());
                    }
                    else if (exception is OperationCanceledException || exception is TimeoutException)
                    {
                        FabricErrorError fabricBackupRestoreError = new FabricErrorError(new FabricError()
                        {
                            Code = NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT,
                            Message = StringResources.RequestTimeout
                        });
                        throw new HttpResponseException(fabricBackupRestoreError.ToHttpResponseMessage());
                    }
                    else if (exception is FabricBackupRestoreLocalRetryException)  //TODO Remove this and change to correct message
                    {
                        if (this._retryCount >= MaxRetryCount)
                        {
                            throw;
                        }
                    }
                    else
                    {
                        var stringBuilder = new StringBuilder();
                        if (exception is AggregateException)
                        {
                            var aggregateException = (AggregateException) exception;
                            foreach (var innerException in aggregateException.InnerExceptions)
                            {
                                stringBuilder.AppendFormat("{0} ,", innerException.Message).AppendLine();
                            }
                        }
                        else
                        {
                            stringBuilder.AppendFormat("{0}", exception.Message);
                        }
                        FabricErrorError fabricBackupRestoreError = new FabricErrorError(new FabricError()
                        {
                            Message =  stringBuilder.ToString(),
                            Code = NativeTypes.FABRIC_ERROR_CODE.E_UNEXPECTED
                        });
                        throw new HttpResponseException(fabricBackupRestoreError.ToHttpResponseMessage());
                    }
                }
            } while (this._retryCount <= MaxRetryCount);
            throw new HttpResponseException(HttpStatusCode.InternalServerError);
        }

        #region Private Method
        private async Task ValidateInput()
        {
            if (!this.ModelState.IsValid)
            {
                this.LogErrorInModels();
                var badRequestMessage = this.Request.CreateErrorResponse(HttpStatusCode.BadRequest,
                    this.ModelState);
                var badRequestMessageDetails = await badRequestMessage.Content.ReadAsStringAsync();
                var fabricBackupRestoreInputValidationError =new FabricErrorError(new FabricError
                    {
                        Message =  badRequestMessageDetails.Replace(@"""",""),
                        Code =  NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG
                    });
                throw new HttpResponseException(fabricBackupRestoreInputValidationError.ToHttpResponseMessage());
            }
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
            BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Validation Failed for Request {0} with Errors : {1} ",
               this.GetType()  , errorStringBuilder.ToString());
        }
        #endregion

        internal void LogException(String traceType , Exception exception)
        {
            if (exception is AggregateException)
            {
                BackupRestoreTrace.TraceSource.WriteWarning(traceType, "Aggregate Exception Stack Trace : {0}", exception.StackTrace);
                var aggregateException = (AggregateException) exception;
                foreach (var innerException in aggregateException.InnerExceptions)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(traceType,
                        "Inner Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        innerException.InnerException, innerException.Message, innerException.StackTrace);

                }
            }
            else
            {
                BackupRestoreTrace.TraceSource.WriteWarning(traceType,
                        "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        exception.InnerException, exception.Message, exception.StackTrace);
            }
        }

        internal string GetFabricRequestFromRequstHeader()
        {
            string fabricRequest;
            try
            {
                
                var headers = Request.Headers.GetValues(BackupRestoreEndPointsConstants.FabricUriHeaderKey);
                fabricRequest = headers.FirstOrDefault();

            }
            catch (Exception)
            {
                var fabricError = new FabricError
                {
                    Message = StringResources.InvalidRequestHeader,
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG
                };

                throw new HttpResponseException( new FabricErrorError(fabricError).ToHttpResponseMessage() );
            }

            return fabricRequest;
        }
    }
}