this is cloud-based I/O burst buffer

OVERVIEW:
--------------------------------------------------------------------------------------------------------------

1. cloud-based I/O burst buffer is a distruted burst buffer system to burst I/O throughpput in cloud environment.

2. there are three kinds of nodes in cloud-based I/O burst buffer:
	Master node:
		manage file meta data.
		manage I/O nodes.
	
	I/O node:
		store real data.
		transfer data with clients.
	
	Client node:
		run user applications.


PORT USED:
--------------------------------------------------------------------------------------------------------------
### Master node:
		port 9000
	
### I/O node:
		port 8000, 8001

HOW TO USE:
--------------------------------------------------------------------------------------------------------------
### 1. build:

Master node:

		1. cd burstbuffer
		2. make Master

I/O node:

		1. cd burstbuffer
		2. make IOnode

Client node:

		1. cd burstbuffer
		2. make Client

all:
		1. cd burstbuffer
		2. make all

### 2. set environment value:

Common:

		1. BURSTBUFFER_HOME
			burstbuffer directory path

Master node:

		1. CBB_MASTER_MOUNT_POINT
			absolute directory path where master node mount shared storage.
			
		2. LD_LIBRARY_PATH
			add $BURSTBUFFER_HOME/lib to LD_LIBRARY_PATH

I/O node:

		1. CBB_IONODE_MOUNT_POINT
			absolute directory path where I/O node mount shared storage.

		2. CBB_MASTER_IP:
			master node ip address.

		3. LD_LIBRARY_PATH
			add $BURSTBUFFER_HOME/lib to LD_LIBRARY_PATH
	
Client node:

		1. CBB_CLIENT_MOUNT_POINT
			expected absolute directory path where client mount shared storage, no need to actually mount.

		2. CBB_MASTER_IP:
			master node ip address.

		3. LD_LIBRARY_PATH
			add $BURSTBUFFER_HOME/lib to LD_LIBRARY_PATH

### 3. start server:
Master node:

	$BURSTBUFFER_HOME/bin/Master

I/O node:

	$BURSTBUFFER_HOME/bin/IOnode

### 4. run test:
on client nodes:

test case:

		cd tests/test{1,2,3,4}
		make
		LD_PRELOAD=$BURSTBUFFER_HOME/lib/libCBB.so ./test{1,2,3,4}

### 5. run your application:
on client nodes:

		LD_PRELOAD=$BURSTBUFFER_HOME/lib/libCBB.so your_application