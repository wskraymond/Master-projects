CC = gcc
all: server client NBserver NBclient getHostByName getHostByAddr getHostName getPeerName

server: server.c
	$(CC) -o $@ $<

clinet: clinet.c
	$(CC) -o $@ $<

NBserver: NBserver.c
	$(CC) -o $@ $<

NBclient: NBclient.c
	$(CC) -o $@ $<

getHostByName: getHostByName.c
	$(CC) -o $@ $<

getHostByAddr: getHostByAddr.c
	$(CC) -o $@ $<

getHostName: getHostName.c
	$(CC) -o $@ $<

getPeerName: getPeerName.c
	$(CC) -o $@ $<

clean:
	rm server client NBserver NBclient getHostByName getHostByAddr getHostName getPeerName
