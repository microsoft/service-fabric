// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using System.Threading.Tasks;


    /// <summary>
    /// Simple Http Listener for working with http
    /// </summary>
    public class SimpleHttpListener
    {
        private readonly HttpListener listener;
        private readonly Func<HttpListenerContext, Task> requestHandler;
        private readonly bool isParallelProcessingSupported;

        public SimpleHttpListener(Uri uri, bool isParallelProcessingSupported, Func<HttpListenerContext, Task> requestHandler)
            : this(new Uri[] { uri }, isParallelProcessingSupported, requestHandler)
        {

        }

        public SimpleHttpListener(IEnumerable<Uri> uris, bool isParallelProcessingSupported, Func<HttpListenerContext, Task> requestHandler)
        {
            // using (new LogHelper("SimpleHttpListener.Constructor"))
            {
                this.isParallelProcessingSupported = isParallelProcessingSupported;
                this.requestHandler = requestHandler;

                this.listener = new HttpListener();

                foreach (var item in uris)
                {
                    string uri = item.ToString();
                    if (!uri.EndsWith("/"))
                    {
                        uri = uri + "/";
                    }

                    // LogHelper.Log("Adding prefix: {0}", uri);
                    this.listener.Prefixes.Add(uri);
                }

            }
        }

        public void Start()
        {
            // LogHelper.Log("SimpleHttpListener - start");
            this.listener.Start();

            this.ProcessRequestLoop();
        }

        public void Stop()
        {
            // LogHelper.Log("SimpleHttpListener - stop");
            this.listener.Stop();
        }

        private void ProcessRequestLoop()
        {
            Task.Factory.FromAsync<HttpListenerContext>(this.listener.BeginGetContext, this.listener.EndGetContext, null).ContinueWith((t) =>
                {
                    if (t.IsFaulted)
                    {
                        // LogHelper.Log("SimpleHttpListener: Failed to read a request: {0}", t.Exception.InnerException);
                        return;
                    }

                    this.requestHandler(t.Result).ContinueWith((requestHandlerTask) =>
                        {
                            if (requestHandlerTask.IsFaulted)
                            {
                                //LogHelper.Log("SimpleHttpListener: RequestHandlerTask faulted with {0}", requestHandlerTask.Exception.InnerException);
                                //LogHelper.Log("SimpleHttpListener: shutting request loop");
                                return;
                            }

                            if (!this.isParallelProcessingSupported)
                            {
                                this.ProcessRequestLoop();
                            }
                        });

                    if (this.isParallelProcessingSupported)
                    {
                        this.ProcessRequestLoop();
                    }
                }, TaskContinuationOptions.ExecuteSynchronously);
        }
    }
}