<!--
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */
-->

##This is HuronFS (Hierarchical, User-level and ON-demand FileSystem

OVERVIEW:
--------------------------------------------------------------------------------------------------------------

![Picture1.jpg](https://bitbucket.org/repo/8xdeRp/images/4105842247-Picture1.jpg)

1. HuronFS is an update of CloudBB, an on-demand distributed burst buffer system to accelerate I/O. HuronFS supports the general TCP/IP protocol that can be widely used. HuronFS also supports high performance network such as Infiniband via the Common Communication Interface ([CCI](http://cci-forum.com/)) framework. CCI framework is a High-Performance Communication Interface for HPC and Data Centers. The CCI project is an open-source communication interface that aims to provide a simple and portable API, high performance, scalability for the largest deployments, and robustness in the presence of faults. It is developed and maintained by a partnership of research, academic, and industry members.

2. HuronFS consists of the following three kinds of nodes:

    * Master node:
Master nodes are the metadata servers.
HuronFS supports multiple Master nodes to distribute workload and avoid bottleneck.
Master nodes manage file meta data and I/O nodes.
A "Master" application is running on each Master node.

    * IOnode:
Under each Master node, there are multiple IOnodes.
IOnodes are responsible for storing actual data and exchanging data with client.
An "IOnode" application is running on each I/O node.

    * Client node:
Client nodes run user applications.
User mounts HuronFS via FUSE or uses a pre-load library to interact with HuronFS.

### Structure of the Cloud-based burst buffer
* Master node:
    * Master: located in bin, is a binary executable file of Master application.
* IOnode:
    * IOnode: located in bin, is a binary executable file of IOnode application.
* Client node:
    * libHuFS.so: located in lib, is a dynamic (preloadable) library used to interact with Master nodes and IOnodes.
    * hufs: located in bin, is a binary executable file using fuse.

PREPARATION:
--------------------------------------------------------------------------------------------------------------
### PORT USED
Can be changed in src/Common/include/CBB_const.h

In TCP/IP mode 

Master node:

* IN: port 9000

IOnode:

* IN: port 7000
* OUT: port 7001

In CCI mode

Master node:

* IN: port 9001

IOnode:

* IN: port 9002

### MOUNT SHARED STORAGE

* Mount shared storage on Master node and IOnode.
* No need to mount shared storage on Client node, but a directory should be selected as shared storage mount point, and set corresponding environment value(HOW TO USE 2.set environment value).

HOW TO USE:
--------------------------------------------------------------------------------------------------------------
### 1. build:
just like other standard UNIX systems, using TCP/IP by default.
--enable-cci=yes --with-cci=CCI_DIR in configure to enable CCI

        ./bootstrap.sh
        ./configure
        make -j
        make install

### 2. Set environment value:

* Common:

    * ####HUFS_HOME
HuronFS directory path

* On master node:

    * ####HUFS_MASTER_MOUNT_POINT
The absolute directory path where master node mounts shared storage.

    * ####HUFS_MASTER_IP_LIST
The IP list of Master nodes, **IMPORTANT.**
If you have multiple Masters, separate them by using commas.
In the case of Infiniband, please use the address of IPoIB. The IP address will be used to initialize the communication, then further communication will use ibverbs via CCI.
Please make sure that the list itself and order of the list are the same across all Master nodes and Clients.

    * ####HUFS_MASTER_BACKUP_POINT
The absolute directory path where master backup metadata for fault tolerance. Use the node local path for better performance.

* On IOnode:

    * ####HUFS_IONODE_MOUNT_POINT
The absolute directory path where IOnode mounts shared storage.

    * ####HUFS_MASTER_IP
The IP address of master node that IOnode belongs to.
In case of Infiniband, please use the IPoIB address

* On Client node:

    * ####HUFS_CLIENT_MOUNT_POINT
The absolute directory path where client mounts HuronFS.

    * ####HUFS_MASTER_IP_LIST
The IP list of Master nodes, **IMPORTANT.**
If you have multiple Masters, separate them by using commas.
In the case of Infiniband, please use the address of IPoIB. The IP address will be used to initialize the communication, then further communication will use ibverbs via CCI.
Please make sure that the list itself and order of the list are the same across all Master nodes and Clients.

    * ####HUFS_STREAM_USE_BUFFER
Set to "true" to use Client local IO buffer for better performance.
Set to "false" to disable, then all the IO will be committed to remote immediately.
**RECOMMENDED TO SET TO TRUE WHILE USING FUSE.**

### 3. start system:
Master node:

        ${HUFS_HOME}/bin/Master

IOnode:

        ${HUFS_HOME}/bin/IOnode

Client:

        ${HUFS_HOME}/bin/hufs

### 4. run test:
On client nodes:

test case:
       
       cd tests/test{1,2,3,4}
       make
       ./prepare.sh
       ./test{1,2,3,4}

### 5. run your application:
On client nodes:

* using FUSE:

    run it as usual

* using LD_PRELOAD:

        LD_PRELOAD=${HUFS_HOME}/lib/libHuFS.so your_application
