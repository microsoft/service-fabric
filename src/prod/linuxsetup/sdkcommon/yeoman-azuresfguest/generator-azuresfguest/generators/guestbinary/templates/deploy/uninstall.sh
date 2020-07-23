#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


azure servicefabric application delete fabric:/<%= appName %>
azure servicefabric application type unregister <%= appTypeName %> 1.0.0