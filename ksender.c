#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define SEQ 2
#define DATA 4
#define LEN 1
#define TYPE 3
#define tCHECK t.payload + (t.payload + LEN - 1)
#define MARK t.payload[LEN] + 1

int send_and_wait(msg *y) {

    msg *r = NULL;
    int contor_timeout = 0;
    while(r==NULL || r->payload[TYPE] != 'Y' || r->payload[SEQ] != y->payload[SEQ]){
        send_message(y);
        r = receive_message_timeout(5000);
    
        if(r==NULL){ // pachet pierdut
            contor_timeout++;
            if(contor_timeout == 3){
                printf("[sender] 3 timeouturi consecutive\n");
                return 0;
            }

        } else  // pachet primit
            contor_timeout = 0;
    }
    return 1;
}

int main(int argc, char** argv) {
    msg t;
    unsigned short crc;
    int data_len;
    init(HOST, PORT);

// pachet S
    t.len = 18;
    t.payload[0] = 0x01;
    t.payload[LEN] = 16;
    t.payload[SEQ] = 0;
    t.payload[TYPE] = 'S';
    t.payload[MARK] = 0x0d;
    memset(t.payload + DATA, 0, 11);
    t.payload[DATA + 0] = 250;
    t.payload[DATA + 1] = 5;
    t.payload[DATA + 4] = 0x0d;
    crc = crc16_ccitt(t.payload, t.len - 3);
    memcpy(t.payload + (t.len - 3), &crc, 2);

    if(!send_and_wait(&t)){
        printf("3 timouts\n");
        return -1;
    }

    printf("[%s] Am trimis corect pachetul S\n", argv[0]);


    for(int i = 1; i < argc; i++) {

        //pachet file header
        data_len = strlen(argv[i]);
        t.len = data_len + 7;
        t.payload[LEN] = t.len - 2;
        t.payload[SEQ] = inc_seq(t.payload[SEQ]);
        t.payload[TYPE] = 'F';
        sprintf(t.payload + DATA, "%s", argv[i]);
        crc = crc16_ccitt(t.payload, t.len - 3);
        memcpy(t.payload + (t.len - 3), &crc, 2);
        t.payload[t.len] = 0x0D;

        if(!send_and_wait(&t)){
            printf("3 timouts\n");
            return -1;
        }

        FILE *f = fopen(argv[i], "rb");

        char buffer[250];
        // pachetele data pt un fisier
        while ((data_len = fread(buffer, 1, sizeof(buffer), f))) {
            //data_len = strlen(buffer);
            t.len = data_len + 7;
            t.payload[LEN] = t.len - 2;
            t.payload[SEQ] = inc_seq(t.payload[SEQ]);
            t.payload[TYPE] = 'D';
            memcpy(t.payload + DATA, buffer, data_len);
            crc = crc16_ccitt(t.payload, t.len - 3);
            memcpy(t.payload + (t.len - 3), &crc, 2);
            t.payload[t.len] = 0x0D;

            if(!send_and_wait(&t)){
                printf("3 timouts\n");
                return -1;
            }
        }

        // end of file
        fclose(f);
        data_len = 0;
        t.len = data_len + 7;
        t.payload[LEN] = t.len - 2;
        t.payload[SEQ] = inc_seq(t.payload[SEQ]);
        t.payload[TYPE] = 'Z';
        crc = crc16_ccitt(t.payload, t.len - 3);
        memcpy(t.payload + (t.len - 3), &crc, 2);
        t.payload[t.len] = 0x0D;

        if(!send_and_wait(&t)){
            printf("3 timouts\n");
            return -1;
        }
    }

//  end of transmission
    t.payload[SEQ] = inc_seq(t.payload[SEQ]);
    t.payload[TYPE] = 'B';
    crc = crc16_ccitt(t.payload, t.len - 3);
    memcpy(t.payload + (t.len - 3), &crc, 2);

    if(!send_and_wait(&t)){
        printf("3 timouts\n");
        return -1;
    }

    return 0;
}