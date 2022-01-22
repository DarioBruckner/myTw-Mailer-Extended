echo execute before: export ldapuser=USERNAME
echo execute before: export ldappw=PW
echo use "set" to check the values
echo adapt ldap.conf: cp ./ldap.conf /etc/ldap/ldap.conf
echo -----------------------------------------------


ldapsearch -x -LLL '(uid=if19b00*)' -D"uid=$ldapuser,ou=people,dc=technikum-wien,dc=at" -w $ldappw dn cn -ZZ