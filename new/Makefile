#Makefile for myTW-Mailer
all: test

test: myldap.cpp
	g++ -std=c++11 -Wall -o myldap myldap.cpp -lldap -llber

clean:
	rm -f ldap
