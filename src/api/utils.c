#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include "utils.h"

const char DATE_FORMAT[28] = "2011-10-08T07:07:09.123456Z";

char* date_iso_now() {
    time_t now = time(NULL);

    int milliseconds;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    milliseconds = lrint(tv.tv_usec/1000.);
    
    int len = strlen(DATE_FORMAT);

    char* buff = malloc(len + sizeof(char));
    strftime(buff, len + sizeof(char), "%Y-%m-%dT%H:%M:%S", gmtime(&now));
    sprintf(buff + strlen("2011-10-08T07:07:09"), ".%06ldZ", tv.tv_usec);
    buff[len] = '\0';
    return buff;
}

char* itoa(int num) {
    int length = snprintf(NULL, 0, "%d", num)+sizeof(char);
    char* buff = malloc(length);
    snprintf(buff, length, "%d", num);
    return buff;
}
