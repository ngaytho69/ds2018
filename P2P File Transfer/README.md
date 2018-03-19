P2P FileTransfer
===============================
Basically, we aim to implement file transfer using socket programming 
Although this code was taken a lot from the internet, we all deeply read and modify (a little bit) the code and try to discuss its component. As a result, we all understand the concept of P2P File transfer.

===================================
How To Compile:
gcc -o server server.c –pthread
./server [server-port-number]

Open other terminal:
gcc –o peer peer.c –pthread
cd peer1
./peer [server-host-name] [server-port-number] [peer-port-number]

===================================
Commands 
	
- list : to get the list of the files in the folder.
- download fileindex : to download the file 
- exit : to quit
