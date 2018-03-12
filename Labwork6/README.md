# Report 6

## Install GlusterFS

Add the GlusterFS PPA repository (version 3.10)

```
sudo add-apt-repository ppa:gluster/glusterfs-3.10
sudo apt-get update
```

Install GlusterFS package.

```
sudo apt-get install -y glusterfs-server
```


## Create a Trusted Pool

Get the IP Address.

```
hostname -I
```

Add the server to the trust pool

```
sudo gluster peer probe <ip-address>
```

Check the peer status

```
sudo gluster peer status
```

## Create a Distributed Replicated Volume

### Add a Storage

For server and each node, create a folder to act as a brick.

### Setup a Volume

```
sudo mkdir /home/<USERNAME>/gluster/test
```

Create a Distributed Replicated Volume.

```
sudo gluster volume create <VOLUME NAME> replica <INT> transport tcp <HOST NAME>:<BRICK PATH> force
```

For instance:

```
sudo gluster volume create test replica 2 transport tcp 192.168.0.160:/home/hung/gluster/test 192.168.0.175/home/lezardvaleth97/gluster/test force
```
Start the Volume.

```
sudo gluster volume start <volume name>
sudo gluster volume start test
```

Check the Volume status.
```
sudo gluster volume info <volume name>
```

## Setup the Client

Install the GlusterFS Client.

```
sudo apt-get install -y glusterfs-client
```

Create a directory to mount the GlusterFS filesystem

Mount the GlusterFS filesystem to the Client.

```
sudo mount -t glusterfs <HOST IP ADDRESS>:/<VOLUME NAME> <DIRECTORY>
```

For instance:

```
sudo mount -t glusterfs 192.168.0.185:/test /mnt
```

Verify the mounted GlusterFS filesystem.

```
# df -h <mount directory>
```

