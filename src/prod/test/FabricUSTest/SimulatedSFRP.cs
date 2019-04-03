// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricUS.Test
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.Collections;
    using System.Fabric.UpgradeService;
    using System.Linq;
    using System.Net.Http;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Collections.Generic;
    using System.IO;
    using System;

    internal class SimulatedSFRP : HttpMessageHandler, IEnumerator<HttpContent>
    {
        List<Tuple<HttpRequestMessage, string>> currentClientMessages = new List<Tuple<HttpRequestMessage, string>>();
        StreamRequest currentStreamRequest = null;
        HttpResponseMessage currentHttpResponseMessage = null;
        IEnumerator<HttpContent> ienumerator = null;

        public SimulatedSFRP()
        {
            ienumerator = GetContent().GetEnumerator();
        }

        #region Json settings
        internal static JsonSerializerSettings GetDefaultInteralSerializationSettings()
        {
            var serializationSettings = new JsonSerializerSettings();
            serializationSettings.NullValueHandling = NullValueHandling.Ignore;
            serializationSettings.Converters.Add(new StringEnumConverter() { CamelCaseText = false });
            serializationSettings.Converters.Add(new VersionConverter());
            serializationSettings.TypeNameHandling = TypeNameHandling.Auto;
            serializationSettings.ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor;
            serializationSettings.ReferenceLoopHandling = ReferenceLoopHandling.Serialize;
            serializationSettings.Context = new StreamingContext(StreamingContextStates.All);

            return serializationSettings;
        }
        #endregion

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            string content = null;
            if (request.Content != null)
            {
                content = await request.Content.ReadAsStringAsync();
            }
            currentClientMessages.Add(Tuple.Create(request, content));
            var response = request.CreateResponse();
            response.Content = Current;
            return response;
        }

        internal IEnumerable<HttpContent> GetContent()
        {
            while (true)
            {
                if (currentStreamRequest != null)
                    yield return new StringContent(GetStreamRequestContentAsync().Result);
                if (currentHttpResponseMessage != null)
                    yield return currentHttpResponseMessage.Content;
            }
        }

        private async Task<string> GetStreamRequestContentAsync()
        {
            using (MemoryStream ms = new MemoryStream())
            {
                var sw = new StreamWriter(ms);
                var requestString = JsonConvert.SerializeObject(currentStreamRequest);
                var requestLength = requestString.Length;
                var lengthArrary = new char[4];
                for (var i = 1; i <= 4; i++)
                {
                    lengthArrary[i - 1] = (char)(requestLength & 0xFF);
                    requestLength = requestLength >> (i * 8);
                }

                await sw.WriteAsync(lengthArrary);
                await sw.FlushAsync();

                await sw.WriteAsync(requestString);
                await sw.FlushAsync();

                ms.Seek(0, SeekOrigin.Begin);
                var sr = new StreamReader(ms);
                var ss = sr.ReadToEnd();
                return ss;
            }
        }

        public bool MoveNext(StreamRequest request)
        {
            currentStreamRequest = request;
            currentHttpResponseMessage = null;
            return MoveNext();
        }

        public bool MoveNext(HttpResponseMessage response)
        {
            currentHttpResponseMessage = response;
            currentStreamRequest = null;
            return MoveNext();
        }

        public bool VerifyResponse(int index, StreamRequest expectedMessage)
        {
            while (currentClientMessages.Count - 1 < index)
            {
                currentClientMessages.Add(null);
            }

            if (expectedMessage == null)
            {
                return currentClientMessages[index] == null;
            }
            else
            {
                if (currentClientMessages[index] == null)
                {
                    return false;
                }
                else
                {
                    var clientRequest = currentClientMessages[index];
                    if (string.Compare(expectedMessage.ClusterQueryUri.TrimEnd('/'), clientRequest.Item1.RequestUri.ToString().TrimEnd('/'), true) != 0)
                    {
                        return false;
                    }

                    if (string.Compare(expectedMessage.Method, clientRequest.Item1.Method.Method, true) != 0)
                    {
                        return false;
                    }

                    if (string.Compare(clientRequest.Item1.Method.Method, "GET", true) != 0)
                    {
                        string requestString = clientRequest.Item1.Content == null ? null : clientRequest.Item2;

                        if (string.Compare(requestString, expectedMessage.Content, true) != 0)
                        {
                            return false;
                        }
                    }

                    if (expectedMessage.Headers != null)
                    {
                        foreach (var header in expectedMessage.Headers)
                        {
                            if (!clientRequest.Item1.Headers.Contains(header.Key))
                            {
                                return false;
                            }
                            else
                            {
                                var value = clientRequest.Item1.Headers.GetValues(header.Key).First();
                                if (string.Compare(header.Value, value, true) != 0)
                                {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }

            return true;
        }

        #region  IEnumerator
        public bool MoveNext()
        {
            var ret = ienumerator.MoveNext();
            Current = ienumerator.Current;
            return ret;
        }

        public HttpContent Current
        {
            get;
            set;
        }

        object IEnumerator.Current
        {
            get
            {
                return Current;
            }
        }

        void IEnumerator.Reset()
        {
            ienumerator = GetContent().GetEnumerator();
        }
        #endregion
    }
}