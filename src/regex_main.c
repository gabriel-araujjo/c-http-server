#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>

int main(int argc, char** argv) {
    char buff[300] = "POST /clientes/75678/transacoes HTTP/1.1\r\n"
    "Host: localhost:8090\r\n"
    "User-Agent: curl/7.68.0\r\n"
    "Accept: */*\r\n"
    "Content-Length: 15\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n";

    regex_t regex;
    regmatch_t matches[2];

    regcomp(&regex, "POST /clientes/([0-9]{1,})/transacoes", REG_EXTENDED);

    if (regexec(&regex, buff, 2, matches, 0) == 0) {
        buff[matches[1].rm_eo] = '\0';
        printf("match: %s\n", buff + matches[1].rm_so);
        printf("match len: %ld\n", (matches[1].rm_eo - matches[1].rm_so) / sizeof(char));
    }

    regfree(&regex);

    size_t s = strlen("\r\n");
    printf("len: %ld\n", s);

    // char buff2[300] = "POST /clientes/75678/transacoes HTTP/1.1\r\n"
    // "Host: localhost:8090\r\n"
    // "User-Agent: curl/7.68.0\r\n"
    // "Accept: */*\r\n"
    // "Content-Length: 15\r\n"
    // "Content-Type: application/x-www-form-urlencoded\r\n"
    // "\r\n"
    // "{\"key\":\"value\"}";
    char buff2[300] = "POST /clientes/75678/transacoes HTTP/1.1\r\n"
    "Host: localhost:8090\r\n"
    "User-Agent: curl/7.68.0\r\n"
    "Accept: */*\r\n"
    "Content-Length: 15\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "\r\n"
    "{\"valor\":1000000,\"tipo\":\"c\",\"descricao\":\"teste test\"}";

    // regcomp(&regex, "User-Agent: ([A-Za-z0-9\\./]{1,})", REG_EXTENDED);
    regcomp(&regex, "\r\n(\\{[\"a-zA-Z0-9:,_ ]{1,}\\}$)", REG_EXTENDED);
    // regcomp(&regex, "\r\n(\\{.*\\})", REG_EXTENDED);

    if (regexec(&regex, buff2, 2, matches, 0) == 0) {
        buff2[matches[1].rm_eo] = '\0';
        printf("match: %s\n", buff2 + matches[1].rm_so);
        printf("match len: %ld\n", (matches[1].rm_eo - matches[1].rm_so) / sizeof(char));
    }

    regfree(&regex);
    
    return 0;
}
