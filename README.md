##This is HuronFS (Hierarchical, User-level and ON-demand FileSystem

OVERVIEW:
--------------------------------------------------------------------------------------------------------------

![Picture1.jpg](https://bitbucket.org/repo/8xdeRp/images/4105842247-Picture1.jpg)

1. HuronFS is an on-demand distributed burst buffer system to accelerate I/O. HuronFS supports the general TCP/IP protocol that can be widely used. HuronFS also supports high performance network such as Infiniband via the CCI framework. HuronFS is an update of CloudBB.

2. HuronFS consists of the following three kind of nodes:

    * Master node:
Master nodes are the metadata servers.
HuronFS supports multiple Master nodes to distribute workload and avoid bottleneck.
Master nodes manage file meta data and I/O nodes.
A "Master" application is running on each Master node.

    * IOnode:
Under each Master node, there are multiple IOnodes
IOnodes are responsible for storing actual data and transferring data with client.
An "IOnode" application is running, on each I/O node.

    * Client node:
Client nodes run user applications.
User could mount HuronFS via FUSE or use a pre-load library to interact with HuronFS.

### Structure of the Cloud-based burst buffer
* Master node:
    * Master: locates in bin, is a binary executable file of Master application.
* IOnode:
    * IOnode: locates in bin, is a binary executable file of IOnode application.
* Client node:
    * libHuFS.so: locates in lib, is a dynamic pre-load library used to interact with Master node and I/O nodes.
    * hufs: locates in bin, is a binary executable file using fuse.

PREPARATION:
--------------------------------------------------------------------------------------------------------------
### PORT USED
Can be changed in src/Common/include/CBB_const.h

In TCP/IP mode 

Master node:

* port 9000, 9001

IOnode:

* port 7000, 7001, 9001

In CCI mode

Master node:

* port 9001

IOnode:

* port 9001

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

    * ####CBB_MASTER_MOUNT_POINT
The absolute directory path where master node mounts shared storage.

    * ####CBB_MASTER_IP_LIST
The ip list of Master nodes, **IMPORTANT.**
If you have multiple Masters, separate them by comma.
In the case of Infiniband, please use the address of IPoIB. The address will be used to initialize the communication when using infiniband, the communication after will use infiniband.
Please make sure that the list itself and order of the list are the same across all Master nodes and Clients.

    * ####CBB_MASTER_BACKUP_POINT
The absolute directory path where master backup metadata for fault tolerance. Use the node local path for better performance.

* On IOnode:

    * ####CBB_IONODE_MOUNT_POINT
The absolute directory path where IOnode mounts shared storage.

    * ####CBB_MASTER_IP
The ip address of master node that IOnode belongs to.
In case of Infiniband please use the IPoIB address

* On Client node:

    * ####CBB_CLIENT_MOUNT_POINT
The absolute directory path where client mounts HuronFS.

    * ####CBB_MASTER_IP_LIST
The ip list of Master nodes, **IMPORTANT.**
If you have multiple Masters, separate them by comma.
In the case of Infiniband, please use the address of IPoIB. The address will be used to initialize the communication when using infiniband, the communication after will use infiniband.
Please make sure that the list itself and order of the list are the same across all Master nodes and Clients.

    * ####CBB_STREAM_USE_BUFFER
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