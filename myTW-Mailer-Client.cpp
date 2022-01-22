#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <arpa/inet.h>

///////////////////////////////////////////////////////////////////////////////

#define BUF 4096
#define PORT 6543

///////////////////////////////////////////////////////////////////////////////

// Prints the usage of the program
void print_usage(char *programm_name)
{
   printf("Usage: %s <ip> <port>\n\n", programm_name);
   EXIT_FAILURE;
}

// Asks the user for a username till a valid username is entered
std::string get_username()
{
   std::string username;
   bool valid;
   do
   {
      valid = true;
      printf("Username Guidelines: 1 to 8 chars [a-z, 0-9]\n");
      std::getline(std::cin, username);
      // Checks if the length is valid
      if (username.length() < 1 || username.length() > 8)
         valid = false;
      else
      {
         // Checks each character
         for (std::string::size_type i = 0; i < username.size(); i++)
         {
            // checks if the character is in range of 0-9 or a-z
            if ((username[i] < 48 || username[i] > 57) && (username[i] < 97 || username[i] > 122))
               valid = false;
         }
      }
   } while (!valid);
   return username;
}

// Reads a SEND message
std::string read_send_message()
{
   std::string line;
   std::stringstream ss;
   ss << "SEND\n";

   // Senders username
   printf("Your username:\n");
   ss << get_username() << "\n";

   // Recievers username
   printf("Recievers username:\n");
   ss << get_username() << "\n";

   // Subject
   printf("Enter a subject\n");
   do
   {
      printf("Subject Guideline: 1 to 80 chars\n");
      std::getline(std::cin, line);
   } while (line.length() < 1 || line.length() > 80);
   ss << line << "\n";

   // Message
   printf("Enter the message (To finish entering put a dot -> '.' in a single line):\n");
   do
   {
      std::getline(std::cin, line);
      ss << line << "\n";
   } while (line != ".");

   std::cout << "\n\n"
             << ss.str() << "\n";
   return ss.str();
}

std::string read_list_message()
{
   std::stringstream ss;
   ss << "LIST\n";

   // Username
   printf("Enter the username from which the messages should be listed:\n");
   ss << get_username() << "\n";

   std::cout << "\n\n"
             << ss.str() << "\n";
   return ss.str();
}

// Logic for delete and read
// Gets a valid username and valid message number
std::string delete_read_logic()
{
   std::stringstream ss;
   std::string line;
   int msg_num;

   // Username
   printf("Enter the username:\n");
   ss << get_username() << "\n";

   // Message number
   printf("Enter the message number of the message\n");
   do
   {
      printf("Message-Number Guideline: Integer greater than 0\n");
      std::getline(std::cin, line);
      std::stringstream intPort(line);
      intPort >> msg_num;
   } while (msg_num <= 0);
   ss << line << "\n";
   return ss.str();
}

// Gets username and message number of delete_read_logic
std::string read_read_message()
{
   std::stringstream ss;
   ss << "READ\n";
   ss << delete_read_logic();

   std::cout << "\n\n"
             << ss.str() << "\n";
   return ss.str();
}

// Gets username and message number of delete_read_logic
std::string read_del_message()
{
   std::stringstream ss;
   ss << "DEL\n";
   ss << delete_read_logic();

   std::cout << "\n\n"
             << ss.str() << "\n";
   return ss.str();
}

int main(int argc, char **argv)
{
   int create_socket;
   char buffer[BUF];
   struct sockaddr_in address;
   int size;
   int status;
   char *programm_name;
   programm_name = argv[0];
   std::string ip;
   std::string result;
   unsigned short port;
   std::string line;

   // Gets IP and Port if the correct number of arguments is given
   if (argc == 3)
   {
      ip = argv[1];
      std::stringstream intPort(argv[2]);
      intPort >> port;
   }
   else
   {
      print_usage(programm_name);
   }

   // Checks if the port is valid
   if (port <= 0 || port > 65535)
   {
      printf("Invalid Port!\n");
      print_usage(programm_name);
   }

   // Checks if the ip address is valid
   struct sockaddr_in sa;
   if (inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) != 1)
   {
      printf("Invalid IP-Adress!\n");
      print_usage(programm_name);
   }

   // Prints the selectet IP and Port
   std::cout << "IP: " << ip << "\n";
   printf("Port: %d\n", port);

   ////////////////////////////////////////////////////////////////////////////
   // CREATE A SOCKET
   // https://man7.org/linux/man-pages/man2/socket.2.html
   // https://man7.org/linux/man-pages/man7/ip.7.html
   // https://man7.org/linux/man-pages/man7/tcp.7.html
   // IPv4, TCP (connection oriented), IP (same as server)
   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket error");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // INIT ADDRESS
   // Attention: network byte order => big endian
   memset(&address, 0, sizeof(address)); // init storage with 0
   address.sin_family = AF_INET;         // IPv4
   // https://man7.org/linux/man-pages/man3/htons.3.html
   address.sin_port = htons(port);
   // https://man7.org/linux/man-pages/man3/inet_aton.3.html
   inet_aton(ip.c_str(), &address.sin_addr);

   ////////////////////////////////////////////////////////////////////////////
   // CREATE A CONNECTION
   // https://man7.org/linux/man-pages/man2/connect.2.html
   if (connect(create_socket,
               (struct sockaddr *)&address,
               sizeof(address)) == -1)
   {
      // https://man7.org/linux/man-pages/man3/perror.3.html
      perror("Connect error - no server available");
      return EXIT_FAILURE;
   }

   // ignore return value of printf
   printf("Connection with server (%s) established\n",
          inet_ntoa(address.sin_addr));

   ////////////////////////////////////////////////////////////////////////////
   // RECEIVE DATA
   // https://man7.org/linux/man-pages/man2/recv.2.html
   size = recv(create_socket, buffer, BUF - 1, 0);
   if (size == -1)
   {
      perror("recv error");
   }
   else if (size == 0)
   {
      printf("Server closed remote socket\n"); // ignore error
   }
   else
   {
      buffer[size] = '\0';
      printf("%s", buffer); // ignore error
   }

   // check for the commands till quit command is inserted
   // readirects to the selected command method
   do
   {
      std::cout << "\n\nUse one of the following commands: (SEND, LIST, READ, DEL, QUIT)\n\n";
      std::getline(std::cin, line);
      if (line == "SEND")
      {
         result = read_send_message();
         status = 1;
      }
      else if (line == "LIST")
      {
         result = read_list_message();
         status = 1;
      }
      else if (line == "READ")
      {
         result = read_read_message();
         status = 1;
      }
      else if (line == "DEL")
      {
         result = read_del_message();
         status = 1;
      }
      else if (line == "QUIT")
      {
         printf("Exit programm..\n");
         status = -1;
      }
      else
      {
         printf("Command not recogniced!\n");
         status = -1;
      }

      if (status != -1)
      {
         char buffer[BUF];
         strcpy(buffer, result.c_str());
         int size = strlen(buffer);
         // remove new-line signs from string at the end
         if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
         {
            size -= 2;
            buffer[size] = 0;
         }
         else if (buffer[size - 1] == '\n')
         {
            --size;
            buffer[size] = 0;
         }

         //////////////////////////////////////////////////////////////////////
         // SEND DATA
         // https://man7.org/linux/man-pages/man2/send.2.html
         // send will fail if connection is closed, but does not set
         // the error of send, but still the count of bytes sent
         if ((send(create_socket, buffer, size, 0)) == -1)
         {
            // in case the server is gone offline we will still not enter
            // this part of code: see docs: https://linux.die.net/man/3/send
            // >> Successful completion of a call to send() does not guarantee
            // >> delivery of the message. A return value of -1 indicates only
            // >> locally-detected errors.
            // ... but
            // to check the connection before send is sense-less because
            // after checking the communication can fail (so we would need
            // to have 1 atomic operation to check...)
            perror("send error");
            break;
         }

         //////////////////////////////////////////////////////////////////////
         // RECEIVE FEEDBACK
         // consider: reconnect handling might be appropriate in somes cases
         //           How can we determine that the command sent was received
         //           or not?
         //           - Resend, might change state too often.
         //           - Else a command might have been lost.
         //
         // solution 1: adding meta-data (unique command id) and check on the
         //             server if already processed.
         // solution 2: add an infrastructure component for messaging (broker)
         //
         size = recv(create_socket, buffer, BUF - 1, 0);
         if (size == -1)
         {
            perror("recv error");
            break;
         }
         else if (size == 0)
         {
            printf("Server closed remote socket\n"); // ignore error
            break;
         }
         else
         {
            buffer[size] = '\0';
            // printf("<< %s\n", buffer); // ignore error

            std::string ret(buffer, strlen(buffer));
            std::cout << ret;
            if (strcmp("OK", ret.substr(0, 2).c_str()) != 0)
            {
               fprintf(stderr, "<< Server error occured, abort\n");
               break;
            }
         }
      }

   } while (line != "QUIT");

   ////////////////////////////////////////////////////////////////////////////
   // CLOSES THE DESCRIPTOR
   if (create_socket != -1)
   {
      if (shutdown(create_socket, SHUT_RDWR) == -1)
      {
         // invalid in case the server is gone already
         perror("shutdown create_socket");
      }
      if (close(create_socket) == -1)
      {
         perror("close create_socket");
      }
      create_socket = -1;
   }

   return EXIT_SUCCESS;
}
