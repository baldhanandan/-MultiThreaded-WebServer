objects = main.o server.o senddata.o manageRequest.o

httpserver: $(objects)
	g++ -o httpserver $(objects) -lpthread

main.o: main.h server.h manageRequest.h

server.o: server.h main.h manageRequest.h

senddata.o: senddata.h manageRequest.h main.h

manageRequest.o: manageRequest.h main.h senddata.h


.PHONY: clean
clean:
	-rm edit $(objects)
