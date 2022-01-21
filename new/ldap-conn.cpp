#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ldap.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <string>

class Ldap
{       // The class
public: // Access specifier
    bool login(std::string username, std::string password)
    { // Method/function defined inside the class
        std::string message_response = "ERR\n";
        ////////////////////////////////////////////////////////////////////////////
        // LDAP config
        // anonymous bind with user and pw empty
        const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
        const int ldapVersion = LDAP_VERSION3;

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
        message_response = "OK\n";
        std::cout << message_response << std::flush;
        // username = incoming_command.username;
        return true;
    }
};

int main()
{
    Ldap myObj;          // Create an object of MyClass
    myObj.login("if21b132", "6G!QftfY"); // Call the method

    return 0;
}