#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


azure servicefabric application package copy <%= appPackage %> fabric:ImageStore
azure servicefabric application type register <%= appPackage %>
azure servicefabric application create fabric:/<%= appName %>  <%= appTypeName %> 1.0.0
