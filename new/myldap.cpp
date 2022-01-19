#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ldap.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
   ////////////////////////////////////////////////////////////////////////////
   // LDAP config
   // anonymous bind with user and pw empty
   const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
   const int ldapVersion = LDAP_VERSION3;

   // read username (bash: export ldapuser=<yourUsername>)
   char ldapBindUser[256];
   char rawLdapUser[128];
   if (argc >= 3 && strcmp(argv[1], "--user") == 0)
   {
      strcpy(rawLdapUser, argv[2]);
      sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUser);
      printf("user set to: %s\n", ldapBindUser);
   }
   else
   {
      const char *rawLdapUserEnv = getenv("ldapuser");
      if (rawLdapUserEnv == NULL)
      {
         printf("(user not found... set to empty string)\n");
         strcpy(ldapBindUser, "");
      }
      else
      {
         sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUserEnv);
         printf("user based on environment variable ldapuser set to: %s\n", ldapBindUser);
      }
   }

   // read password (bash: export ldappw=<yourPW>)
   char ldapBindPassword[256];
   if (argc == 4 && strcmp(argv[3], "--pw") == 0)
   {
      char *password=getpass("Enter password: ");
      strcpy(ldapBindPassword, password);
      printf("pw taken over from commandline\n");
   }
   else
   {
      const char *ldapBindPasswordEnv = getenv("ldappw");
      if (ldapBindPasswordEnv == NULL)
      {
         strcpy(ldapBindPassword, "");
         printf("(pw not found... set to empty string)\n");
      }
      else
      {
         strcpy(ldapBindPassword, ldapBindPasswordEnv);
         printf("pw taken over from environment variable ldappw\n");
      }
   }

   // search settings
   const char *ldapSearchBaseDomainComponent = "dc=technikum-wien,dc=at";
   const char *ldapSearchFilter = "(uid=if21b132*)";
   ber_int_t ldapSearchScope = LDAP_SCOPE_SUBTREE;
   const char *ldapSearchResultAttributes[] = {"uid", "cn", NULL};

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
      return EXIT_FAILURE;
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
      return EXIT_FAILURE;
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
      return EXIT_FAILURE;
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
      fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
      ldap_unbind_ext_s(ldapHandle, NULL, NULL);
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // perform ldap search
   // https://linux.die.net/man/3/ldap_search_ext_s
   // _s : synchronous
   // int ldap_search_ext_s(
   //     LDAP *ld,
   //     char *base,
   //     int scope,
   //     char *filter,
   //     char *attrs[],
   //     int attrsonly,
   //     LDAPControl **serverctrls,
   //     LDAPControl **clientctrls,
   //     struct timeval *timeout,
   //     int sizelimit,
   //     LDAPMessage **res );
   LDAPMessage *searchResult;
   rc = ldap_search_ext_s(
       ldapHandle,
       ldapSearchBaseDomainComponent,
       ldapSearchScope,
       ldapSearchFilter,
       (char **)ldapSearchResultAttributes,
       0,
       NULL,
       NULL,
       NULL,
       500,
       &searchResult);
   if (rc != LDAP_SUCCESS)
   {
      fprintf(stderr, "LDAP search error: %s\n", ldap_err2string(rc));
      ldap_unbind_ext_s(ldapHandle, NULL, NULL);
      return EXIT_FAILURE;
   }
   // https://linux.die.net/man/3/ldap_count_entries
   printf("Total results: %d\n", ldap_count_entries(ldapHandle, searchResult));

   ////////////////////////////////////////////////////////////////////////////
   // get result of search
   // https://linux.die.net/man/3/ldap_first_entry
   // https://linux.die.net/man/3/ldap_next_entry
   LDAPMessage *searchResultEntry;
   for (searchResultEntry = ldap_first_entry(ldapHandle, searchResult);
        searchResultEntry != NULL;
        searchResultEntry = ldap_next_entry(ldapHandle, searchResultEntry))
   {
      /////////////////////////////////////////////////////////////////////////
      // Base Information of the search result entry
      // https://linux.die.net/man/3/ldap_get_dn
      printf("DN: %s\n", ldap_get_dn(ldapHandle, searchResultEntry));

      /////////////////////////////////////////////////////////////////////////
      // Attributes
      // https://linux.die.net/man/3/ldap_first_attribute
      // https://linux.die.net/man/3/ldap_next_attribute
      //
      // berptr: berptr, a pointer to a BerElement it has allocated to keep
      //         track of its current position. This pointer should be passed
      //         to subsequent calls to ldap_next_attribute() and is used to
      //         effectively step through the entry's attributes.
      BerElement *ber;
      char *searchResultEntryAttribute;
      for (searchResultEntryAttribute = ldap_first_attribute(ldapHandle, searchResultEntry, &ber);
           searchResultEntryAttribute != NULL;
           searchResultEntryAttribute = ldap_next_attribute(ldapHandle, searchResultEntry, ber))
      {
         BerValue **vals;
         if ((vals = ldap_get_values_len(ldapHandle, searchResultEntry, searchResultEntryAttribute)) != NULL)
         {
            for (int i = 0; i < ldap_count_values_len(vals); i++)
            {
               printf("\t%s: %s\n", searchResultEntryAttribute, vals[i]->bv_val);
            }
            ldap_value_free_len(vals);
         }

         // free memory
         ldap_memfree(searchResultEntryAttribute);
      }
      // free memory
      if (ber != NULL)
      {
         ber_free(ber, 0);
      }

      printf("\n");
   }

   // free memory
   ldap_msgfree(searchResult);

   ////////////////////////////////////////////////////////////////////////////
   // https://linux.die.net/man/3/ldap_unbind_ext_s
   // int ldap_unbind_ext_s(
   //       LDAP *ld,
   //       LDAPControl *sctrls[],
   //       LDAPControl *cctrls[]);
   ldap_unbind_ext_s(ldapHandle, NULL, NULL);

   return EXIT_SUCCESS;
}