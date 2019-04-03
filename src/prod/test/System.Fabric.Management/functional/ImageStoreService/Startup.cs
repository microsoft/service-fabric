// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MS.Test.ImageStoreService
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Net;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Diagnostics;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.Configuration;
    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.Extensions.Logging;
    using Microsoft.Extensions.Options;
    using Newtonsoft.Json;

    public class Startup
    {
        public Startup(IConfiguration configuration)
        {
            this.Configuration = configuration;
        }

        public IConfiguration Configuration { get; }

        // This method gets called by the runtime. Use this method to add services to the container.
        public void ConfigureServices(IServiceCollection services)
        {
            ServiceEventSource.Current.Message("In Startup: ConfigureServices");
            services.AddMvc();
        }

        // This method gets called by the runtime. Use this method to configure the HTTP request pipeline.
        public void Configure(IApplicationBuilder app, IHostingEnvironment env)
        {
            ServiceEventSource.Current.Message("In Startup: Configure");

            // Add global exception handler.
            app.UseExceptionHandler(
                builder =>
                {
                    builder.Run(
                    async context =>
                    {
                        // serialize the exception and send to client in json.
                        ServiceEventSource.Current.Message("Sending exception to client..");

                        context.Response.StatusCode = (int)HttpStatusCode.InternalServerError;
                        context.Response.ContentType = "application/json";
                        var ex = context.Features.Get<IExceptionHandlerFeature>();
                        if (ex != null)
                        {
                            var err = JsonConvert.SerializeObject(new Error()
                            {
                                // Use ToString to capture all stacktraces of all innerExceptions.
                                Stacktrace = ex.Error.ToString(),
                                Message = ex.Error.Message
                            });

                            await context.Response.Body.WriteAsync(Encoding.ASCII.GetBytes(err), 0, err.Length).ConfigureAwait(false);
                        }
                    });
                });

            // Use MVC for Controllers.
            app.UseMvc();
        }
    }

    public class Error
    {
        public string Message { get; set; }

        public string Stacktrace { get; set; }
    }
}
