#Makefile for myTW-Mailer
all: server client 

server: myTW-Mailer-Server.cpp
	g++ -g -fPIC -Wall -o myTW-Mailer-Server myTW-Mailer-Server.cpp -lstdc++fs -std=c++17 -lldap -llber

client: myTW-Mailer-Client.cpp
	g++ -g -fPIC -Wall -o myTW-Mailer-Client myTW-Mailer-Client.cpp

clean:
	rm -f myTW-Mailer-Server
	rm -f myTW-Mailer-Client
