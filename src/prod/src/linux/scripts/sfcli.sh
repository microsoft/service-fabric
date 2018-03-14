#!/bin/bash

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


if [ $# -ge 1 ]; then
case "$1" in
    copy-application-package)
    copy_application_package=1
    ;;
    test-application-package)
    test_application_package=1
    ;;
    remove-application-package)
    remove_application_package=1
    ;;
    invoke-encrypt-text)
    invoke_encrypt_text=1
    ;;
    invoke-decrypt-text)
    invoke_decrypt_text=1
    ;;
    -h|--help)
    help=1
    ;;
    *)
    ;;
esac
shift
fi

if [ $# -le 0 ] || [ -n "$help" ]; then
    echo 'Service Fabric Cli'
    echo 'Usage:'
    echo '    $ ./sfcli.sh [action]'
    echo '        Actions:'
    echo '        copy-application-package [parameters]'
    echo '            Parameters:'
    echo '            --connection-endpoint [hostname:port]'
    echo '            --application-package-path [application_package_path]'
    echo '            --image-store-connection-string [image_store_connection_string]'
    echo '            --application-package-path-in-image-store [application_package_path_in_image_store]'
    echo '            --certificate-fingerprint [certificate_fingerprint]'
    echo ''
    echo '        test-application-package [parameters]'
    echo '            Parameters:'
    echo '            --connection-endpoint [hostname:port]'
    echo '            --application-package-path [application_package_path]'
    echo '            --application-parameter [application_parameter]'
    echo '            --image-store-connection-string [image_store_connection_string]'
    echo '            --certificate-fingerprint [certificate_fingerprint]'
    echo ''
    echo '        remove-application-package [parameters]'
    echo '            Parameters:'
    echo '            --connection-endpoint [hostname:port]'
    echo '            --image-store-connection-string [image_store_connection_string]'
    echo '            --application-package-path-in-image-store [application_package_path_in_image_store]'
    echo '            --certificate-fingerprint [certificate_fingerprint]'
    echo ''
    echo '        invoke-encrypt-text [parameters]'
    echo '            Parameters:'
    echo '            --text [text]'
    echo '            --certificate-path [certificate_path]'
    echo '            --algorithm-oid [algorithm_oid]'
    echo ''
    echo '        invoke-decrypt-text [parameters]'
    echo '            Parameters:'
    echo '            --cipher-text [cipher_text]'
    echo ''
    echo 'Examples:'
    echo '    $ ./sfcli.sh copy-application-package --connection-endpoint localhost:10549 --application-package-path /tmp/app --image-store-connection-string fabric:ImageStore'
    echo ''
    exit 0;
fi

while [[ $# -ge 1 ]]
do
key="$1"
case $key in
    --connection-endpoint)
    connection_endpoint="$2"
    shift
    ;;
    --certificate-fingerprint)
    certificate_fingerprint="$2"
    shift
    ;;
    --application-package-path)
    application_package_path="$2"
    shift
    ;;
    --image-store-connection-string)
    image_store_connection_string="$2"
    shift
    ;;
    --application-package-path-in-image-store)
    application_package_path_in_image_store="$2"
    shift
    ;;
    --application-parameter)
    application_parameter="$2"
    shift
    ;;
    --text)
    text="$2"
    shift
    ;;
    --certificate-path)
    certificate_path="$2"
    shift
    ;;
    --algorithm-oid)
    algorithm_oid="$2"
    shift
    ;;
    --cipher-text)
    cipher_text="$2"
    shift
    ;;
    *)
    ;;
esac
shift
done

service_fabric_code_path='/opt/microsoft/servicefabric/bin/Fabric/Fabric.Code'
if [ -n $SERVICE_FABRIC_DEVELOPER ]; then
    service_fabric_code_path=$SERVICE_FABRIC_CODE_PATH
fi
export LD_LIBRARY_PATH=$service_fabric_code_path

if [ -n "$copy_application_package" ]; then
    if [ -z "$connection_endpoint" ] || [ -z "$application_package_path" ] || [ -z "$image_store_connection_string" ]; then
        echo 'The parameters connection_endpoint, application_package_path, image_store_connection_string are required.'
        exit 1;
    fi
    ${service_fabric_code_path}/AzureCliProxy.sh copyapplicationpackage "$connection_endpoint" "$application_package_path" "$image_store_connection_string" "$application_package_path_in_image_store" "$certificate_fingerprint"
elif [ -n "$test_application_package" ]; then
    if [ -z "$connection_endpoint" ] || [ -z "$application_package_path" ] || [ -z "$image_store_connection_string" ]; then
        echo 'The parameters connection_endpoint, application_package_path, image_store_connection_string are required.'
        exit 1;
    fi
    ${service_fabric_code_path}/AzureCliProxy.sh testapplicationpackage "$connection_endpoint" "$application_package_path" "$application_parameter" "$image_store_connection_string" "$certificate_fingerprint"
elif [ -n "$remove_application_package" ]; then
    if [ -z "$connection_endpoint" ] || [ -z "$image_store_connection_string" ] || [ -z "$application_package_path_in_image_store" ]; then
        echo 'The parameters connection_endpoint, image_store_connection_string, application_package_path_in_image_store are required.'
        exit 1;
    fi
    ${service_fabric_code_path}/AzureCliProxy.sh removeapplicationpackage "$connection_endpoint" "$image_store_connection_string" "$application_package_path_in_image_store" "$certificate_fingerprint"
elif [ -n "$invoke_encrypt_text" ]; then
    if [ -z "$text" ] || [ -z "$certificate_path" ]; then
        echo 'The parameters text, certificate_path are required.'
        exit 1;
    fi
    ${service_fabric_code_path}/AzureCliProxy.sh invokeencrypttext "$text" "$certificate_path" "$algorithm_oid"
elif [ -n "$invoke_decrypt_text" ]; then
    if [ -z "$cipher_text" ]; then
        echo 'The parameter cipher_text is required.'
        exit 1;
    fi
    ${service_fabric_code_path}/AzureCliProxy.sh invokedecrypttext "$cipher_text"
fi