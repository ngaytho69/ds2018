#rpcgen -C square.x
#cc -c client.c -o client.o
#cc -c square_clnt.c -o square_clnt.o
#cc -c square_xdr.c -o square.xdr.o
#cc -o client client.o square_clnt.o square_xdr.o
#cc -c client.c server.c square_xdr.c
#cc -c server.c -o server.o
#cc -c square_svc.c -o square_svc.o
#cc -o server server.o square_svc.o square_xdr.o