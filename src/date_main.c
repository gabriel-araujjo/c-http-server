#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

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

int main(int argc, char** argv) {
    // time_t now = time(NULL);

    // int milliseconds;
    // struct timeval tv;

    // gettimeofday(&tv, NULL);
    // printf("%ld\n", tv.tv_usec);
    // milliseconds = lrint(tv.tv_usec/1000.);
    // printf("%d\n", milliseconds);
    // if (milliseconds >= 1000) {
    //     milliseconds -= 1000;
    //     tv.tv_sec++;
    // }

    // char buff[sizeof "2011-10-08T07:07:09.123456Z"];
    // strftime(buff, sizeof buff, "%Y-%m-%dT%H:%M:%S", gmtime(&now));
    // printf("Hello world at: %s.%06ldZ\n\n", buff, tv.tv_usec);

    char* dt = date_iso_now();
    printf("date iso now: %s\n\n", dt);
    free(dt);
    return 0;
}