#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

#
# This file sets the environment to be used for building with clang.
#

export CC=$(which clang)
export CXX=$(which clang++)