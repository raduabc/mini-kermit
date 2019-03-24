#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001
#define SEQ 2
#define DATA 4
#define LEN 1
#define TYPE 3
#define rCHECK r->payload + (r->len - 3)

int main(int argc, char** argv) {
	msg *r, t;
	init(HOST, PORT);

	unsigned short crc, exit=1, contor_timeout = 0;
	char expected_seq = 1, last_seq = 0;
	
	for(int i = 0; i < 3 && exit == 1; i++){
		r = receive_message_timeout(5000);
		if(r != NULL) {
			memcpy(&crc, rCHECK, 2);
			if(crc == crc16_ccitt(r->payload, r->len - 3))
				exit=0;
			else i = 0;
		}
		else printf("[%s] %d timeout(s)\n", argv[0], i);
	}

	if(exit == 1){
		printf("[%s] 3 timeouturi consecutive\n", argv[0]);
		return -1;
	}
	printf("[%s] Pachet S primit cu succes\n", argv[0]);
	memcpy(t.payload, r->payload, r->len);
	t.payload[TYPE] = 'Y';
	send_message(&t);


	FILE *f = NULL;;
	char buffer[250];

	while(1){
		r = receive_message_timeout(5000);
		if(r == NULL){ // timeout
			printf("[%s] timeout\n", argv[0]);
			contor_timeout++;
			if(contor_timeout == 3){
				printf("[%s] timeout exit\n", argv[0]);
				if(f != NULL) // daca eram in timpul scrierii 
					fclose(f); // inchid fisierul
				return-1;
			}

		} else { 
			// mesaj primit
			contor_timeout = 0;
			if(r->payload[SEQ] != expected_seq){ // secventa gresita
				printf("[%s] secventa gresita: %d asteptat: %d\n", argv[0], r->payload[SEQ], expected_seq);
				t.payload[SEQ] = last_seq;
				send_message(&t); //trimit confirmare pt ultima
			} else { 
				// secventa buna
				memcpy(&crc, rCHECK, 2);
				
				if(crc != crc16_ccitt(r->payload, r->len - 3)) { // crc gresit
					t.payload[SEQ] = expected_seq;
					t.payload[TYPE] = 'N';
					printf("[%s] NAK %d\n", argv[0], expected_seq);
					send_message(&t);
				} else { 
					// mesaj corect
					t.payload[SEQ] = expected_seq;
					t.payload[TYPE] = 'Y';
					printf("[%s] pachet %c seq %d\n", argv[0], r->payload[TYPE], expected_seq);
					send_message(&t);

					last_seq = expected_seq;
					expected_seq = inc_seq(expected_seq);
					memset(buffer, 0, sizeof(buffer));
					
					switch(r->payload[TYPE]) {
						case 'F':
							strcpy(buffer, "recv_");
							memcpy(buffer + 5, r->payload + DATA, r->len - 7);
							f = fopen(buffer, "wb");
							printf("[%s] %s\n", argv[0], buffer);
							break;
						case 'Z':
							fclose(f);
							f = NULL;
							break;
						case 'B':
							return 0;
						case 'D':
							memcpy(buffer, r->payload + DATA, r->len - 7);
							fwrite(buffer, 1, r->len - 7, f);
							break;
					}

				}
			}
		}
	}
}
