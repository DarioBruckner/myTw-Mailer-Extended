#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ldap.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <string>
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
#include "jsoncpp/jsoncpp.cpp"

class Commands
{ // The class
private:
    // checks if the Datastructe has been already used or not
    void checkifDatastructExsists(std::string location)
    {
        FILE *file;
        if ((file = fopen(location.c_str(), "r")))
        {
            fclose(file);
        }
        else
        {
            file = fopen(location.c_str(), "w");
            fputs("{\"inbox\" : []}", file);
            fclose(file);
        }
    }
    // writes String into the given file
    bool writetoFile(std::string location, std::string content)
    {
        std::ofstream datastr(location);
        if (datastr.is_open())
        {
            datastr << content;
            datastr.close();
            return true;
        }
        else
        {
            datastr.close();
            return false;
        }
    }


    bool matchStringWithRegex(std::string testString){
        return std::regex_match(testString, username_regex);

    }

public: // Access specifier
    std::string Directory = "";
    std::string CurrentUser = "";
    static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
    std::regex username_regex("^[a-zA-Z0-9]*$");

    // splits the string at the given character
    std::string SplitString(std::string &str, const char c)
    {
        const size_t index = str.find(c);
        const std::string ret = str.substr(0, index);
        str.erase(0, index + 1);
        return ret;
    }


    void process_lock(void)
    {
        int ret;

	    ret = pthread_mutex_lock(&thread_mutex);
        if (ret != 0)
                _exit(EXIT_FAILURE);
    }

    void process_unlock(void)
    {
        int ret;

	    ret = pthread_mutex_unlock(&thread_mutex);
        if (ret != 0)
                _exit(EXIT_FAILURE);
    }

    // Trys to login a user using ldap
    bool Login(std::string userInput)
    { // Method/function defined inside the class
        ////////////////////////////////////////////////////////////////////////////
        // LDAP config
        // anonymous bind with user and pw empty
        const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
        const int ldapVersion = LDAP_VERSION3;
        const std::string username = SplitString(userInput, '\n'); // username
        const std::string password = SplitString(userInput, '\n'); // password
        // read username (bash: export ldapuser=<yourUsername>)
        char ldapBindUser[256];
        char rawLdapUser[128];
        strcpy(rawLdapUser, username.c_str());
        sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUser);

        // read password (bash: export ldappw=<yourPW>)
        char ldapBindPassword[256];
        strcpy(ldapBindPassword, password.c_str());

        // general
        int rc = 0; // return code

        ////////////////////////////////////////////////////////////////////////////
        // setup LDAP connection
        // https://linux.die.net/man/3/ldap_initialize
        LDAP *ldapHandle;
        rc = ldap_initialize(&ldapHandle, ldapUri);
        if (rc != LDAP_SUCCESS)
        {
            fprintf(stderr, "ldap_init failed\n");
            return false;
        }
        printf("connected to LDAP server %s\n", ldapUri);

        ////////////////////////////////////////////////////////////////////////////
        // set verison options
        // https://linux.die.net/man/3/ldap_set_option
        rc = ldap_set_option(
            ldapHandle,
            LDAP_OPT_PROTOCOL_VERSION, // OPTION
            &ldapVersion);             // IN-Value
        if (rc != LDAP_OPT_SUCCESS)
        {
            // https://www.openldap.org/software/man.cgi?query=ldap_err2string&sektion=3&apropos=0&manpath=OpenLDAP+2.4-Release
            fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            return false;
        }

        ////////////////////////////////////////////////////////////////////////////
        // start connection secure (initialize TLS)
        // https://linux.die.net/man/3/ldap_start_tls_s
        // int ldap_start_tls_s(LDAP *ld,
        //                      LDAPControl **serverctrls,
        //                      LDAPControl **clientctrls);
        // https://linux.die.net/man/3/ldap
        // https://docs.oracle.com/cd/E19957-01/817-6707/controls.html
        //    The LDAPv3, as documented in RFC 2251 - Lightweight Directory Access
        //    Protocol (v3) (http://www.faqs.org/rfcs/rfc2251.html), allows clients
        //    and servers to use controls as a mechanism for extending an LDAP
        //    operation. A control is a way to specify additional information as
        //    part of a request and a response. For example, a client can send a
        //    control to a server as part of a search request to indicate that the
        //    server should sort the search results before sending the results back
        //    to the client.
        rc = ldap_start_tls_s(
            ldapHandle,
            NULL,
            NULL);
        if (rc != LDAP_SUCCESS)
        {
            fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            return false;
        }

        ////////////////////////////////////////////////////////////////////////////
        // bind credentials
        // https://linux.die.net/man/3/lber-types
        // SASL (Simple Authentication and Security Layer)
        // https://linux.die.net/man/3/ldap_sasl_bind_s
        // int ldap_sasl_bind_s(
        //       LDAP *ld,
        //       const char *dn,
        //       const char *mechanism,
        //       struct berval *cred,
        //       LDAPControl *sctrls[],
        //       LDAPControl *cctrls[],
        //       struct berval **servercredp);

        BerValue bindCredentials;
        bindCredentials.bv_val = (char *)ldapBindPassword;
        bindCredentials.bv_len = strlen(ldapBindPassword);
        BerValue *servercredp; // server's credentials
        rc = ldap_sasl_bind_s(
            ldapHandle,
            ldapBindUser,
            LDAP_SASL_SIMPLE,
            &bindCredentials,
            NULL,
            NULL,
            &servercredp);
        if (rc != LDAP_SUCCESS)
        {
            fprintf(stderr, "LDAP bind error: %s\n%d\n", ldap_err2string(rc), rc);
            ldap_unbind_ext_s(ldapHandle, NULL, NULL);
            return false;
        }
        CurrentUser = ldapBindUser;
        return true;
    }
    // Takes data provieded by the user and adds them to the datastructure
    std::string CreateMessage(std::string userInput)
    {
        // Takes data provieded by the user and adds them to the datastructure
        // const std::string sender = SplitString(userInput, '\n'); // Sender

        const std::string reciever = SplitString(userInput, '\n'); // reciever

        const std::string topic = SplitString(userInput, '\n'); // subject
        if (topic.length() > 80 || sender.length() > 8 || reciever.length() > 8)
        {
            return "ERR\n";
        }

        std::stringstream tmp;
        // filterung \n for read function
        while (userInput.find("\n", 0) != std::string::npos)
        {
            tmp << userInput.substr(0, userInput.find("\n", 0)) << "\\";
            if (userInput.find("\n", 0) + 1 <= userInput.size())
            {
                userInput = userInput.substr(userInput.find("\n", 0) + 1, userInput.size());
            }
            else
            {
                userInput = "";
            }
        }
        tmp << userInput << "\\";
        const std::string content = tmp.str(); // userInput content

        // depending if the user added a / to the directory or not adding a /
        if (Directory.back() != '/')
        {
            Directory.append("/");
        }
        std::string finaldirectory = Directory + reciever;             // directory of the user specific datastructure
        std::string location = finaldirectory + "/datastructure.json"; // path of the datastructure

        // checks if its the first userInput and if the directory exists and if not creats it.
        if (!std::filesystem::exists(finaldirectory))
        {
            if (!std::filesystem::create_directory(finaldirectory))
            {
                std::cout << "Error creating directory" << std::endl;
                return "ERR\n";
            }
        }

        process_lock();

        checkifDatastructExsists(location);

        process_unlock();

        std::ifstream datastructure(location, std::ifstream::binary); // filestream of the datastructure
        Json::Value data;                                             // the data of the datastructre
        datastructure >> data;
        Json::Value newData;    // the new data added to the existing data
        Json::FastWriter write; // json to string converter for easy writing into the file

        newData["sender"] = CurrentUser;
        newData["reciever"] = reciever;
        newData["subject"] = topic;
        newData["userInput"] = content;

        data["inbox"].append(newData);
        std::string output = write.write(data);

        process_lock();
        if (writetoFile(location, output))
        {
            std::cout << "Message successfully saved" << std::endl;
        }
        else
        {
            std::cout << "Error saving Message" << std::endl;
            process_unlock();
            return "ERR\n";
        }
        process_unlock();
        return "OK\n";
    }
    // Reads the desired userInput out of the json file
    std::string GetMessage(std::string userInput)
    {
        Json::StreamWriterBuilder builder;
        Json::Value jsonValues;
        int userInput_id;
        std::stringstream intId(SplitString(userInput, '\n')); // Gets the userInput ID
        intId >> userInput_id;                                 // Converts userInput ID in int
        userInput_id -= 1;                                     // Converts ID to index

        // Trys to read the file
        std::ifstream user_file(Directory + "/" + CurrentUser + "/datastructure.json", std::ifstream::binary);
        if( user.length() > 8 || user_file.fail()){
            return "ERR\n";
        }
        process_lock();

        user_file >> jsonValues;

        process_unlock();

        if (jsonValues.isMember("inbox"))
        {
            const Json::Value inbox = jsonValues["inbox"];
            int size = inbox.size();

            // Read data out of json value
            if (userInput_id < size)
            {
                std::stringstream ss;
                ss << "OK\n";
                ss << Json::writeString(builder, inbox[userInput_id]["sender"]).substr(1, Json::writeString(builder, inbox[userInput_id]["sender"]).size() - 2) << "\n";
                ss << Json::writeString(builder, inbox[userInput_id]["reciever"]).substr(1, Json::writeString(builder, inbox[userInput_id]["reciever"]).length() - 2) << "\n";
                ss << Json::writeString(builder, inbox[userInput_id]["subject"]).substr(1, Json::writeString(builder, inbox[userInput_id]["subject"]).length() - 2) << "\n";
                std::string userInput_text = Json::writeString(builder, inbox[userInput_id]["userInput"]).substr(1, Json::writeString(builder, inbox[userInput_id]["userInput"]).length() - 2);

                // Loop to replace \\ with \n
                std::stringstream tmp;
                while (userInput_text.find("\\", 0) != std::string::npos)
                {
                    tmp << userInput_text.substr(0, userInput_text.find("\\", 0)) << "\n";
                    if (userInput_text.find("\\", 0) + 2 <= userInput_text.size())
                    {
                        userInput_text = userInput_text.substr(userInput_text.find("\\", 0) + 2, userInput_text.size());
                    }
                    else
                    {
                        userInput_text = "";
                    }
                }
                ss << tmp.str() << userInput_text;
                return ss.str();
            }
        }
        return "ERR\n";
    }
    // Lists subjects of all userInputs of the desired user
    std::string ListMessages(std::string userInput)
    {
        Json::Value jsonValues;
        Json::StreamWriterBuilder builder;

        // Trys to read the JSON File
        std::ifstream user_file(Directory + "/" + CurrentUser + "/datastructure.json", std::ifstream::binary);
        if (user_file.fail())
        {
            return "0\n";
        }else if(user.length() > 8){
            return "ERR\n";
        }

        process_lock();

        user_file >> jsonValues;

        process_unlock();

        // Counts the userInputs and returns all the subject titles
        if (jsonValues.isMember("inbox"))
        {
            const Json::Value inbox = jsonValues["inbox"];
            std::stringstream ss;
            int size = inbox.size();
            ss << "OK\n";
            ss << inbox.size() << "\n";
            for (int i = 0; i < size; i++)
            {
                ss << Json::writeString(builder, inbox[i]["subject"]).substr(1, Json::writeString(builder, inbox[i]["subject"]).length() - 2) << "\n";
            }
            return ss.str();
        }
        return "0\n";
    }
    // Deletes a specific userInput
    std::string DeleteMessage(std::string userInput)
    {
        const std::string index = SplitString(userInput, '\n'); // the index of the userInput that it deleted
        std::string finaldirectory = Directory + "/" + CurrentUser;

        if (!std::filesystem::exists(finaldirectory))
        {
            return "ERR\n";
        }

        std::ifstream datastructure(finaldirectory + "/datastructure.json", std::ifstream::binary);

        Json::Value data;
        Json::FastWriter writer;

        process_lock();

        datastructure >> data;

        process_unlock();


        int ind = stoi(index);
        ind--;
        int size = data["inbox"].size();
        if (size == 0 || ind > size)
        {
            return "ERR\n";
        }
        Json::Value new_items;
        new_items["inbox"] = Json::arrayValue;
        for (int i = 0; i < size; i++)
        {
            if (i != ind)
            {
                new_items["inbox"].append(data["inbox"][i]);
            }
        }
        data["inbox"] = new_items["inbox"];

        std::string finaldata = writer.write(data);

        process_lock();

        writetoFile(finaldirectory + "/datastructure.json", finaldata);

        process_unlock();

        return "OK\n";
    }
};