#!/usr/bin/env python

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

import socket

TCP_IP = '127.0.0.1'
TCP_PORT = 40000
BUFFER_SIZE = 1024
MESSAGE = "Hello, World!"

def append_int(arr, val):
    arr.append((val & 0xff))
    arr.append((val & 0xff00) >> 8)
    arr.append((val & 0xff0000) >> 16)
    arr.append((val & 0xff000000) >> 24)

def make_block(block_len):
    arr = bytearray(block_len)
    for i in range(0, block_len):
        arr[i] = 0x7d
    return arr


try:
    print "Sending write requests message to 127.0.0.1"

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))
    arr = bytearray()

    device_id = u"C_DRIVE\0"
    device_id = device_id.encode('utf-16le')

    # mode = write
    append_int(arr, 2)  

    # offset = 0
    append_int(arr, 0)

    # BLOCK_SIZE = 16384
    append_int(arr, 16384)

    # device_id_length
    append_int(arr, len(device_id))

    s.send(arr)

    s.send(device_id)

    s.send(make_block(16384))

    arr = bytearray()
    append_int(arr, 0xd0d0d0d0)
    s.send(arr)

    # padding
    s.send(make_block(200+32+4-4*5-len(device_id)))

    # Receive write repsonse
    print "Receiving write response..."
    ret = s.recv(4096)
    print ret
    for b in ret:
        print hex(ord(b))

    ## s.close()

    ####################################################

    ## s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ## s.connect((TCP_IP, TCP_PORT))
    print "Sending read requests message to 127.0.0.1"
    arr = bytearray()

    device_id = u"C_DRIVE\0"
    device_id = device_id.encode('utf-16le')

    # mode = read 
    append_int(arr, 1)  

    # offset = 0
    append_int(arr, 0)

    # BLOCK_SIZE = 16384
    append_int(arr, 16384)

    # device_id_length
    append_int(arr, len(device_id))

    s.send(arr)

    s.send(device_id)

    s.send(make_block(16384))

    arr = bytearray()
    append_int(arr, 0xd0d0d0d0)
    s.send(arr)
    
    # padding
    s.send(make_block(200+32+4-4*5-len(device_id)))

    # Receive write repsonse
    print "Receiving write response..."
    ret = s.recv(32767)
    print "RESPONSE_LENGTH=", len(ret)
    i = 0
    for b in ret:
        i = i + 1
        if (i % 20 == 0):
            print
        print hex(ord(b)), ' ',

    s.close()
    print "Connection closed"

except Exception as e:
    print e
    if (s):
        s.close()

    
