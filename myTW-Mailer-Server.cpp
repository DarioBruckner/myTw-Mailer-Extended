#include <filesystem>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <regex>
#include <ctime>


//////////////////////////////////////////////////////////////////////////////
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
std::regex username_regex("^[a-zA-Z0-9]*$");  

#include "commands.cpp"

///////////////////////////////////////////////////////////////////////////////

#define BUF 1024
#define PORT 6543

///////////////////////////////////////////////////////////////////////////////

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;
std::string directory = "";
Commands commands;

///////////////////////////////////////////////////////////////////////////////

void signalHandler(int sig);
void *child(void *data);

///////////////////////////////////////////////////////////////////////////////

// Prints usage of programm
void print_usage(char *programm_name)
{
   printf("Usage: %s <ip> <mail-spool-directoryname>\n\n", programm_name);
   EXIT_FAILURE;
}

int main(int argc, char **argv)
{
   pid_t pid;
   socklen_t addrlen;
   struct sockaddr_in address, cliaddress;
   int reuseValue = 1;
   char *programm_name;
   programm_name = argv[0];
   unsigned short port;

   // Gets IP and Port if the correct number of arguments is given
   if (argc == 3)
   {
      if (std::filesystem::exists(argv[2]))
      {
         std::stringstream intPort(argv[1]);
         intPort >> port;
         commands.Directory = argv[2];
      }
      else
      {
         printf("Directory not found \n");
         print_usage(programm_name);
         return EXIT_FAILURE;
      }
   }
   else
   {
      print_usage(programm_name);
      return EXIT_FAILURE;
   }

   // Checks if the port is valid
   if (port <= 0 || port > 65535)
   {
      printf("Invalid Port!\n");
      print_usage(programm_name);

      return EXIT_FAILURE;
   }

   // Prints the selectet Directory and Port
   std::cout << "Directory: " << commands.Directory << "\n";
   printf("Port: %d\n", port);

   ////////////////////////////////////////////////////////////////////////////
   // SIGNAL HANDLER
   // SIGINT (Interrup: ctrl+c)
   // https://man7.org/linux/man-pages/man2/signal.2.html
   if (signal(SIGINT, signalHandler) == SIG_ERR)
   {
      perror("signal can not be registered");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // CREATE A SOCKET
   // https://man7.org/linux/man-pages/man2/socket.2.html
   // https://man7.org/linux/man-pages/man7/ip.7.html
   // https://man7.org/linux/man-pages/man7/tcp.7.html
   // IPv4, TCP (connection oriented), IP (same as client)
   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket error"); // errno set by socket()
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // SET SOCKET OPTIONS
   // https://man7.org/linux/man-pages/man2/setsockopt.2.html
   // https://man7.org/linux/man-pages/man7/socket.7.html
   // socket, level, optname, optvalue, optlen
   if (setsockopt(create_socket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reuseAddr");
      return EXIT_FAILURE;
   }

   if (setsockopt(create_socket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reusePort");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // INIT ADDRESS
   // Attention: network byte order => big endian
   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(port);

   ////////////////////////////////////////////////////////////////////////////
   // ASSIGN AN ADDRESS WITH PORT TO SOCKET
   if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
   {
      perror("bind error");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // ALLOW CONNECTION ESTABLISHING
   // Socket, Backlog (= count of waiting connections allowed)
   if (listen(create_socket, 5) == -1)
   {
      perror("listen error");
      return EXIT_FAILURE;
   }

   while (!abortRequested)
   {
      /////////////////////////////////////////////////////////////////////////
      // ignore errors here... because only information message
      // https://linux.die.net/man/3/printf
      printf("Waiting for connections...\n");

      /////////////////////////////////////////////////////////////////////////
      // ACCEPTS CONNECTION SETUP
      // blocking, might have an accept-error on ctrl+c
      addrlen = sizeof(struct sockaddr_in);
      if ((new_socket = accept(create_socket,
                               (struct sockaddr *)&cliaddress,
                               &addrlen)) == -1)
      {
         if (abortRequested)
         {
            perror("accept error after aborted");
         }
         else
         {
            perror("accept error");
         }
         break;
      }

      /////////////////////////////////////////////////////////////////////////
      // START CLIENT
      // ignore printf error handling
      printf("Client connected from %s:%d...\n",
             inet_ntoa(cliaddress.sin_addr),
             ntohs(cliaddress.sin_port));

      pid = fork();
      switch(pid){
         case -1:
         printf("Child could not be started");
            exit(EXIT_FAILURE);
            break;
         case 0:
            child(&new_socket);
            exit(EXIT_SUCCESS);
            break;
         default:
            break;
      }
      new_socket = -1;
   }

   // frees the descriptor
   if (create_socket != -1)
   {
      if (shutdown(create_socket, SHUT_RDWR) == -1)
      {
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
//gets the index of the ip in the Json array 
int getIndexOfIp(Json::Value data, std::string sip){
   int counter = 0;
   int ret = 0;
   for(Json::Value& val : data["blacklist"]){
      for(const auto& ip : val.getMemberNames()){
         std::string s = ip;
         if(s.compare(sip) == 0){
            ret = counter;
            break;
         }

      }
      counter++;
   }
   return ret;
}

//returns the Ip from the socket as a string
std::string getIpfromSocket(int clientsocket){
   struct sockaddr_in* ipAddres = (struct sockaddr_in*)&create_socket;
   struct in_addr ipAdd = ipAddres->sin_addr;

   char str[INET_ADDRSTRLEN];
   inet_ntop(AF_INET, &ipAdd, str,INET_ADDRSTRLEN);

   std::string s(str);

   return s;
}

//Returns the login Attempts from user that is defined by the Ip-address
int getLoginAttempts(int create_socket){
   

   std::string s = getIpfromSocket(create_socket);

   std::string location = commands.Directory + "/blacklist.json";

   commands.process_lock();
   commands.checkifBlacklistExsists(location);
   commands.process_unlock();

   
   
   std::ifstream blacklist(location, std::ifstream::binary); // filestream of the datastructure
   Json::Value data;                                             // the data of the datastructre
   blacklist >> data;
   Json::Value newData; 
   Json::Value head;
   Json::FastWriter write; // json to string converter for easy writing into the file

   

   bool ismember = false;
   for(const Json::Value& value : data["blacklist"]){
      for(const auto& ip : value.getMemberNames()){
         std::string ipstr = ip;
         if(ipstr.compare(s) == 0){
            ismember = true;
            break;
         }
      }
   }

   

   if(ismember == true){
      int index = getIndexOfIp(data,s);
      int attempts = data["blacklist"][index][s]["Attempts"].asInt();
      return attempts; 
      
   }else{
      newData["Attempts"] = 0;
      newData["Timestamp"] = 0;
      head[s] = newData;

      data["blacklist"].append(head);

      std::string output = write.write(data);

      commands.process_lock();
      commands.writetoFile(location, output);
      commands.process_unlock();

      return 0;
   }

}

//Unblacklist a client once the time has passed
void unblacklistIP(int create_socket){
   std::string s = getIpfromSocket(create_socket);

   std::string location = commands.Directory + "/blacklist.json";

   std::ifstream blacklist(location, std::ifstream::binary); // filestream of the datastructure
   Json::Value data;                                             // the data of the datastructre
   blacklist >> data;
   Json::Value newData; 
   Json::FastWriter write; // json to string converter for easy writing into the file
   
   int index  = getIndexOfIp(data, s);

   data["blacklist"][index][s]["Timestamp"] = 0;
   data["blacklist"][index][s]["Attempts"] = 0;

   std::string output = write.write(data);

   commands.process_lock();
   commands.writetoFile(location, output);
   commands.process_unlock();
}

//Blacklists a IP and makes it impossible to login
void blacklistIP(int create_socket){
   std::string s = getIpfromSocket(create_socket);

   int timestamp = (int)std::time(nullptr);
   
   std::string location = commands.Directory + "/blacklist.json";

   std::ifstream blacklist(location, std::ifstream::binary); // filestream of the datastructure
   Json::Value data;                                             // the data of the datastructre
   blacklist >> data;
   Json::Value newData; 
   Json::FastWriter write; // json to string converter for easy writing into the file
   
   int index  = getIndexOfIp(data, s);

   data["blacklist"][index][s]["Timestamp"] = timestamp;
   data["blacklist"][index][s]["Attempts"] = 0;
   std::string output = write.write(data);

   commands.process_lock();
   commands.writetoFile(location, output);
   commands.process_unlock();
}

// Edits the attempt counter of the IP address either to 0 or add one
void modifyAttempt(int create_socket, bool reset){
   std::string s = getIpfromSocket(create_socket);

   std::string location = commands.Directory + "/blacklist.json";

   std::ifstream blacklist(location, std::ifstream::binary); // filestream of the datastructure
   Json::Value data;                                             // the data of the datastructre
   blacklist >> data;
   Json::Value newData; 
   Json::FastWriter write; // json to string converter for easy writing into the file
   int newattp = 0;
   if(reset){

      int index = getIndexOfIp(data, s);
      int currentTime = data["blacklist"][index][s]["Timestamp"].asInt();
      
      Json::Value update, head;
      update["Attempts"] = 0;
      update["Timestamp"] = currentTime;
      head[s] = update;

      data["blacklist"][index] = head;

      data["blacklist"][index][s]["Attempts"] = 0;
   }else{
      int index = getIndexOfIp(data, s);

      int currentattp = data["blacklist"][index][s]["Attempts"].asInt();
      int currentTime = data["blacklist"][index][s]["Timestamp"].asInt();

      Json::Value update, head;
      newattp = currentattp+1;
      
      

      update["Attempts"] = newattp;
      update["Timestamp"] = currentTime;
      head[s] = update;

      data["blacklist"][index] = head;
   }
   std::string output = write.write(data);

   commands.process_lock();
   commands.writetoFile(location, output);
   commands.process_unlock();
   
   if(newattp == 3){
      blacklistIP(create_socket);
   }
}


bool isBlackListed(int create_socket){
   std::string s = getIpfromSocket(create_socket);

   int timestamp = (int)std::time(nullptr);

   int c = getLoginAttempts(create_socket);  

   if(c >= 3){

      blacklistIP(create_socket);
      std::cout << "BLACKLISTED" << std::endl;
      return true;
   }else{
      std::string location = commands.Directory + "/blacklist.json";

      std::ifstream blacklist(location, std::ifstream::binary); // filestream of the datastructure
      Json::Value data;                                             // the data of the datastructre
      blacklist >> data;
      Json::Value newData; 
      
      int index = getIndexOfIp(data, s);

      if(data["blacklist"][index][s]["Timestamp"] != 0){
         int diff = timestamp - data["blacklist"][index][s]["Timestamp"].asInt();
         if(diff < 60){
            return true;
         }else{
            unblacklistIP(create_socket);

            return false;
         }
      }else{
         return false;
      }  
   }
}




void *child(void *data)
{
   char buffer[BUF];
   char return_buffer[BUF+BUF];
   std::string result;
   int size;
   int *current_socket = (int *)data;
   // FILE * file;
   std::vector<std::string> command;
   std::string word;
   std::string user;

   ////////////////////////////////////////////////////////////////////////////
   // SEND welcome message
   strcpy(buffer, "Welcome to myserver!\r\nPlease enter your commands...\r\n");
   if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
   {
      perror("send failed");
      return NULL;
   }

   do
   {
      

      /////////////////////////////////////////////////////////////////////////
      // RECEIVE
      memset(buffer, 0, BUF);
      size = recv(*current_socket, buffer, BUF - 1, 0);
      if (size == -1)
      {
         if (abortRequested)
         {
            perror("recv error after aborted");
         }
         else
         {
            perror("recv error");
         }
         break;
      }

      if (size == 0)
      {
         printf("Client closed remote socket\n"); // ignore error
         break;
      }

      // remove ugly debug message, because of the sent newline of client
      if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
      {
         size -= 2;
      }
      else if (buffer[size - 1] == '\n')
      {
         --size;
      }

      buffer[size] = '\0';

      std::string userInput = buffer;

      std::string command = commands.SplitString(userInput, '\n');

      
      if(!isBlackListed(create_socket)){
         if (command == "LOGIN"){
            std::cout << "Command" << command << std::endl;
            if (commands.Login(userInput)){
               result = "OK\n";
               modifyAttempt(create_socket, true);
            }else{
               result = "ERR\n";
               modifyAttempt(create_socket, false);
            }
         }else if (commands.CurrentUser.compare("") != 0){
            if (command == "SEND"){
               std::cout << "Command" << command << std::endl;
               result = commands.CreateMessage(userInput);
            }else if (command == "LIST"){
               std::cout << "Command" << command << std::endl;
               result = commands.ListMessages(userInput);
            }else if (command == "READ"){
               std::cout << "Command" << command << std::endl;
               result = commands.GetMessage(userInput);
            }else if (command == "DEL"){
               std::cout << "Command" << command << std::endl;
               result = commands.DeleteMessage(userInput);
            }else{
               printf("Message received: %s\n", buffer); // ignore error
            }
         }else{
            result = "ERR\n";
         }
      }else{
         result = "You are blacklisted\n";
      }
      strcpy(return_buffer, result.c_str());
      send(*current_socket, return_buffer, strlen(return_buffer), 0);

      command.clear();

      
      
   } while (strcmp(buffer, "quit") != 0 && !abortRequested);

   // closes/frees the descriptor if not already
   if (*current_socket != -1)
   {
      if (shutdown(*current_socket, SHUT_RDWR) == -1)
      {
         perror("shutdown new_socket");
      }
      if (close(*current_socket) == -1)
      {
         perror("close new_socket");
      }
      *current_socket = -1;
   }

   return NULL;
}

void signalHandler(int sig)
{
   if (sig == SIGINT)
   {
      printf("abort Requested... "); // ignore error
      abortRequested = 1;
      /////////////////////////////////////////////////////////////////////////
      // With shutdown() one can initiate normal TCP close sequence ignoring
      // the reference count.
      // https://beej.us/guide/bgnet/html/#close-and-shutdownget-outta-my-face
      // https://linux.die.net/man/3/shutdown
      if (new_socket != -1)
      {
         if (shutdown(new_socket, SHUT_RDWR) == -1)
         {
            perror("shutdown new_socket");
         }
         if (close(new_socket) == -1)
         {
            perror("close new_socket");
         }
         new_socket = -1;
      }

      if (create_socket != -1)
      {
         if (shutdown(create_socket, SHUT_RDWR) == -1)
         {
            perror("shutdown create_socket");
         }
         if (close(create_socket) == -1)
         {
            perror("close create_socket");
         }
         create_socket = -1;
      }
   }
   else
   {
      exit(sig);
   }
}
