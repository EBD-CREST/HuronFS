#!/bin/bash

#echo "This is the content of test file1:">$CBB_CLIENT_MOUNT_POINT/test1
dd if=/dev/zero of=$CBB_CLIENT_MOUNT_POINT/test1 bs=100MB count=10
