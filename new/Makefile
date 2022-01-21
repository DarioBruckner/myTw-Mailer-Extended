#Makefile for myTW-Mailer
all: ldap

ldap: ldap-conn.cpp
	g++ -std=c++11 -Wall -o ldap ldap-conn.cpp -lldap -llber

clean:
	rm -f ldap
