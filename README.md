this is cloud-based I/O burst buffer

OVERVIEW:
--------------------------------------------------------------------------------------------------------------

1. Cloud-based I/O burst buffer is a distributed burst buffer system to burst I/O throughput in cloud environment.

2. Cloud-based I/O burst buffer consists of the following kinds of nodes:

	* Master node:
                Master node locates on cloud side, manages file meta data and I/O nodes.
                On Master node, the Master application is running.
	
	* I/O node:
                I/O nodes also locate on cloud side, is responsible for storing real data and transferring data with client.
                On I/O node, the IOnode application is running.
	
	* Client node:
		Client nodes locate on cloud side, running user applications. a pre-load library used to interact with Master node and I/O nodes.

### Structure of the Cloud-based burst buffer
* Master node:
	* Master: locates in bin, binary executable file of Master application.
	* libMaster.so: locates in lib, dynamic library of Master application.
* I/O node:
	* IOnode: locates in bin, binary executable file of IOnode application.
	* libIOnode.so: locates in lib, dynamic library of IOnode application.
* Client node:
	* libCBB.so: locates in lib, dynamic pre-load library used to interact with Master node and I/O nodes.

PREPARATION:
--------------------------------------------------------------------------------------------------------------
### PORT USED
Master node:

		port 9000

I/O node:

		port 8000, 8001

### MOUNT SHARED STORAGE

* Mount shared storage on Master node and I/O node.
* No need to mount shared storage on Client node, but a directory should be selected as shared storage mount point, and set corresponding environment value(HOW TO USE 2.set environment value).

HOW TO USE:
--------------------------------------------------------------------------------------------------------------
### 1. build:

Master:

		1. cd burstbuffer
		2. make Master

I/O node:

		1. cd burstbuffer
		2. make IOnode

Client:

		1. cd burstbuffer
		2. make Client

To make all:

		1. cd burstbuffer
		2. make all

### 2. set environment value:

Common:

		1. BURSTBUFFER_HOME
			burstbuffer directory path

On master node:

		1. CBB_MASTER_MOUNT_POINT:
			absolute directory path where master node mounts shared storage.
			
		2. LD_LIBRARY_PATH:
			add $BURSTBUFFER_HOME/lib to LD_LIBRARY_PATH

On I/O node:

		1. CBB_IONODE_MOUNT_POINT:
			absolute directory path where I/O node mounts shared storage.

		2. CBB_MASTER_IP:
			master node ip address.

		3. LD_LIBRARY_PATH:
			add $BURSTBUFFER_HOME/lib to LD_LIBRARY_PATH
	
On client node:

		1. CBB_CLIENT_MOUNT_POINT:
			expected absolute directory path where client mounts shared storage, no need to actually mount.

		2. CBB_MASTER_IP:
			master node ip address.

		3. LD_LIBRARY_PATH:
			add $BURSTBUFFER_HOME/lib to LD_LIBRARY_PATH

### 3. start server:
Master node:

	$BURSTBUFFER_HOME/bin/Master

I/O node:

	$BURSTBUFFER_HOME/bin/IOnode

### 4. run test:
On client nodes:

test case:

		cd tests/test{1,2,3,4}
		make
		./prepare.sh
		LD_PRELOAD=$BURSTBUFFER_HOME/lib/libCBB.so ./test{1,2,3,4}

### 5. run your application:
On client nodes:

		LD_PRELOAD=$BURSTBUFFER_HOME/lib/libCBB.so your_application
