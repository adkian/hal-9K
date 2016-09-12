
/*
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 */
 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#include "src/record.h"
#include "src/stt.h"

#define SSDP_MULTICAST              "239.255.255.250"
#define SSDP_PORT                   1900

#define BUTTON_HOME                 "Home"
#define BUTTON_REW                  "Rew"
#define BUTTON_FWD                  "Fwd"
#define BUTTON_PLAY                 "Play"
#define BUTTON_SELECT               "Select"
#define BUTTON_LEFT                 "Left"
#define BUTTON_RIGHT                "Right"
#define BUTTON_DOWN                 "Down"
#define BUTTON_UP                   "Up"
#define BUTTON_BACK                 "Back"
#define BUTTON_INSTANTREPLAY	    "InstantReplay"
#define BUTTON_INFO		    "Info"
#define BUTTON_BACKSPACE	    "Backspace"
#define BUTTON_SEARCH		    "Search"
#define BUTTON_ENTER		    "Enter"
#define BUTTON_LIT		    "Lit_"

#define NETFLIX_ID                  "12"
#define SPOTIFY_ID                  "19977"

#define RESPONSE_BUFFER_LEN         8192
#define URL_BUFFER_LEN              1024
char buffer[RESPONSE_BUFFER_LEN];
unsigned int len = RESPONSE_BUFFER_LEN;

char url[URL_BUFFER_LEN];
char host[URL_BUFFER_LEN];


int invocation();
void speak(int);
void getCommand();
int tvCommands(int);
int http_req_resp(char* host, int port, char* req, char** resp);
int ssdp_get_roku_ecp_url(char* url);
void rokuWrite(char *, int, char*);
int sendTVcommand(int, char *, int);

int main(void){
    while(1){
	if(invocation()){
	    speak(1);
	    getCommand();
	}	    	
    }    
}

int invocation(){
    char text[100];
    int invoked=0;
    int res= 0;
    record(2);
    speechToText(text);
    printf("\n\nTEXT: %s\n\n", text);
    if(strstr(text, "HAL PAUSE") || strstr(text, "HAL PLAY"))
	res = tvCommands(5);
    else if(strstr(text, "HAL SKIP"))
	res = tvCommands(6);
    else if(strstr(text, "HAL RESPOND") || strstr(text, "HAL")){
	invoked=1;
    }
    speak(res);
    return invoked;
}

void speak(int num){
    switch(num){
    case 1:
	system("festival --tts speech/greetings");
	break;
    case 2:
	system("festival --tts speech/TVnotConnectedError");
	break;
    }
}

void getCommand(){
    char text[100];
    int res=0;
    record(3);
    speechToText(text);
    printf("\n\nTEXT: %s\n\n", text);
    if(strstr(text, "SKIP"))
    	res = tvCommands(6);
    else if(strstr(text, "NET FLICKS")){
    	if(strstr(text, "STAR TREK"))
    	    res = tvCommands(1);
    }
    else if(strstr(text, "BEETHOVEN SYMPHONIES"))
	res = tvCommands(2);
    else if(strstr(text, "BEETHOVEN PIANO SONATAS"))
	res = tvCommands(3);
    else if(strstr(text, "RANDOM BEETHOVEN"))
	res = tvCommands(4);
    else if(strstr(text, "PAUSE") || strstr(text, "PLAY"))
    	res = tvCommands(5);
    speak(res);
}

/*
  Return values: 
  1 - successful execution
  2 - unable to reach TV
  3 - connection error
*/
int tvCommands(int command){
    int i;
    int ret = -1;
    char* hostbegin;
    char* hostend;
    char* portbegin;
    char* portend;
    char  portStr[10];
    int   port;
    
    /* int netflix_id = 12, spotify_id = 19977; */

    //check for broadcast every 20 mins
    ret = ssdp_get_roku_ecp_url(url);
    if(ret < 0)
	return 2;

    hostbegin = strstr(buffer, "http://") + 7;
    hostend = strstr(hostbegin, ":");
    strncpy(host, hostbegin, hostend-hostbegin);
    host[hostend-hostbegin] = 0;

    portbegin = hostend+1;
    portend = strstr(portbegin, "/");
    strncpy(portStr, portbegin, portend-portbegin);
    portStr[portend-portbegin] = 0;
    port = atoi(portStr);

    sendTVcommand(command, host, port);
}

int ssdp_get_roku_ecp_url(char* url)
{
    int sock;
    size_t ret;
    unsigned int socklen;
    struct sockaddr_in sockname;
    struct sockaddr clientsock;
    struct hostent *hostname = 0;
    char ssdproku[] =
	"M-SEARCH * HTTP/1.1\r\n"
	"Host: 239.255.255.250:1900\r\n"
	"Man: \"ssdp:discover\"\r\n"
	"ST: roku:ecp\r\n"
	"\r\n";
    fd_set fds;
    struct timeval timeout;
    char* urlbegin;
    char* urlend;

    /* SSDP Request */
    hostname = gethostbyname(SSDP_MULTICAST);
    if (!hostname) {

	printf("gethostbyname returns null for host %s", SSDP_MULTICAST);
    }
    hostname->h_addrtype = AF_INET;

    if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
	printf("err: socket() failed");
	return -1;
    }

    memset((char*)&sockname, 0, sizeof(sockname));
    sockname.sin_family=AF_INET;
    sockname.sin_port=htons(SSDP_PORT);
    sockname.sin_addr.s_addr=*((unsigned long*)(hostname->h_addr_list[0]));
    
    ret=sendto(sock, ssdproku, strlen(ssdproku), 0, (struct sockaddr*) &sockname, sizeof(sockname));
    if(ret != strlen(ssdproku)){
	printf("err:sendto");
	return -1;
    }

    /* SSDP Response */
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    timeout.tv_sec=10;
    timeout.tv_usec=10;

    if(select(sock+1, &fds, NULL, NULL, &timeout) < 0){
	printf("err:select");
	return -1;
    }
    if(FD_ISSET(sock, &fds)){
	socklen=sizeof(clientsock);
	if((len = recvfrom(sock, buffer, len, MSG_PEEK, &clientsock, &socklen)) == (size_t)-1){
	    printf("err: recvfrom");
	    return -1;
	}
	buffer[len]='\0';
	//    close(sock);

	if(strncmp(buffer, "HTTP/1.1 200 OK", 12) != 0){
	    printf("err: ssdp parsing ");
	    return -1;
	}

	/* Parse out Location url */
	urlbegin = strstr(buffer, "LOCATION: ") + 10;
	urlend = strstr(urlbegin, "/\r\n");
	strncpy(url, urlbegin, urlend-urlbegin);
	url[urlend-urlbegin] = 0;
    } else {
	printf("err: no ssdp answer");
	return -1;
    }
    return 0;
}

/*
Command dir
1 - Star Trek (Netflix)
2 - Beethoven (Spotify)
3 - Bach (Spotify)
 */
int sendTVcommand(int command, char* host, int port){
    char *resp;

    char enter[] = 
	"POST /keypress/" BUTTON_ENTER  " HTTP/1.0\r\n"
	"\r\n";
    
    char homeKey[] = 
	"POST /keypress/" BUTTON_HOME  " HTTP/1.0\r\n"
	"\r\n";

    char rightKey[] = 
	"POST /keypress/" BUTTON_RIGHT " HTTP/1.0\r\n"
	"\r\n";

    char upKey[] =
	"POST /keypress/" BUTTON_UP " HTTP/1.0\r\n"
	"\r\n";

    char play_pause[] =
	"POST /keypress/" BUTTON_PLAY " HTTP/1.0\r\n"
	"\r\n";

    char downKey[] =
	"POST /keypress/" BUTTON_DOWN " HTTP/1.0\r\n"
	"\r\n";
    
    char launchNetflix[] =
	"POST /launch/" NETFLIX_ID " HTTP/1.0\r\n"
	"\r\n";
    
    char launchSpotify[] =
	"POST /launch/" SPOTIFY_ID " HTTP/1.0\r\n"
	"\r\n";

    char select[] =
	"POST /keypress/" BUTTON_SELECT " HTTP/1.0\r\n"
	"\r\n";

    char fwd[] =
	"POST /keypress/" BUTTON_FWD " HTTP/1.0\r\n"
	"\r\n";	
    int randup, randdown;
    switch(command){
    case 1:
	http_req_resp(host, port, launchNetflix, &resp);
	sleep(10);
	http_req_resp(host, port, upKey, &resp);
	sleep(1);
	http_req_resp(host, port, select, &resp);
	sleep(1);
	http_req_resp(host, port, upKey, &resp);
	sleep(1);
	http_req_resp(host, port, select, &resp);
	sleep(1);
	rokuWrite(host, port, "Star Trek The Next");
	http_req_resp(host, port, enter, &resp);
	sleep(7);
	for(int i=0; i<5; i++)
	    http_req_resp(host, port, rightKey, &resp);
	sleep(1);
	http_req_resp(host, port, select, &resp);
	sleep(4);
	http_req_resp(host, port, select, &resp);
	break;
    case 2:
	http_req_resp(host, port, launchSpotify, &resp);
	sleep(10);
	http_req_resp(host, port, select, &resp);
	http_req_resp(host, port, select, &resp);
	rokuWrite(host, port, "karajan symphony edition");
	sleep(1);
	for(int i=0; i<4; i++)
	    http_req_resp(host, port, rightKey, &resp);
	http_req_resp(host, port, downKey, &resp);
	http_req_resp(host, port, select, &resp);
	srand(time(NULL));
	randup = rand() % 30;
	randdown = rand() % 30;
	for(int i=0; i<randup; i++)
	    http_req_resp(host, port, upKey, &resp);
	for(int i=0; i<randdown; i++)
	    http_req_resp(host, port, downKey, &resp);
	http_req_resp(host, port, select, &resp);
	break;
    case 3:
	http_req_resp(host, port, launchSpotify, &resp);
	sleep(10);
	http_req_resp(host, port, select, &resp);
	http_req_resp(host, port, select, &resp);
	rokuWrite(host, port, "brendel piano sonatas");
	sleep(1);
	for(int i=0; i<4; i++)
	    http_req_resp(host, port, rightKey, &resp);
	http_req_resp(host, port, downKey, &resp);
	http_req_resp(host, port, select, &resp);
	srand(time(NULL));
	randup = rand() % 30;
	randdown = rand() % 30;
	for(int i=0; i<randup; i++)
	    http_req_resp(host, port, upKey, &resp);
	for(int i=0; i<randdown; i++)
	    http_req_resp(host, port, downKey, &resp);
	http_req_resp(host, port, select, &resp);
	break;
    case 4:
	http_req_resp(host, port, launchSpotify, &resp);
	sleep(10);
	http_req_resp(host, port, select, &resp);
	http_req_resp(host, port, select, &resp);
	rokuWrite(host, port, "beethoven");
	sleep(1);
	for(int i=0; i<4; i++)
	    http_req_resp(host, port, rightKey, &resp);
	http_req_resp(host, port, downKey, &resp);
	srand(time(NULL));
	randup = rand() % 30;
	randdown = rand() % 30;
	for(int i=0; i<randdown; i++)
	    http_req_resp(host, port, downKey, &resp);
	for(int i=0; i<randup; i++)
	    http_req_resp(host, port, upKey, &resp);
	http_req_resp(host, port, select, &resp);
	http_req_resp(host, port, select, &resp);
	break;
    case 5:
	http_req_resp(host, port, play_pause, &resp);
	break;
    case 6:
	http_req_resp(host, port, fwd, &resp);
	break;
    }
}

void rokuWrite(char *host, int port, char * text){
    char *resp;
    char select_start[] =
	"POST /keypress/" BUTTON_LIT;
    char select_end[] =
	" HTTP/1.0\r\n"
	"\r\n";
    
    char send_text[50];
    char curr_char[4];
    for(int i = 0; i<strlen(text); i++){
	if(text[i] == ' ')
	    strcpy(curr_char, "%20");
	
	else
	    curr_char[0] = text[i];
	strcat(send_text, select_start);
	strcat(send_text, curr_char);
	strcat(send_text, select_end);
	http_req_resp(host, port, send_text, &resp);
	send_text[0]=0;
    }
}


#define DWORD int
int http_req_resp(char* host, int port, char* req, char** resp)
{
    int sock;
    size_t ret;
    unsigned int socklen;
    struct sockaddr_in sockname;
    struct sockaddr clientsock;
    struct hostent *hostname;
    fd_set fds;
    struct timeval timeout;

    hostname = gethostbyname(host);
    if (hostname == NULL) {
	printf("gethostbyname returns null for host %s", host);
    }
    hostname->h_addrtype = AF_INET;

    if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
	printf("err: socket() failed");
	return -1;
    }

    memset((char*)&sockname, 0, sizeof(sockname));
    sockname.sin_family=AF_INET;
    sockname.sin_port=htons(port);
    sockname.sin_addr.s_addr=*((unsigned long*)(hostname->h_addr_list[0]));

    ret=connect(sock, (struct sockaddr*) &sockname, sizeof(sockname));
    ret=send(sock, req, strlen(req), 0);
    
    //Send will not account for partial data writes -- for now
    if(ret != strlen(req)){	
	printf("err:sendto");
	return -1;
    }

    //Recv will not account for partial data reads -- for now
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    timeout.tv_sec=10;
    timeout.tv_usec=10;

    if(select(sock+1, &fds, NULL, NULL, &timeout) < 0){
	printf("err:select");
	return -1;
    }

    if(FD_ISSET(sock, &fds)){
	if((len = recv(sock, buffer, sizeof(buffer), 0)) == (size_t)-1){
	    printf("err: recvfrom");
	    return -1;
	}
	buffer[len]='\0';
	//    close(sock);

	if(strncmp(buffer+9, "200 OK", 6) != 0){
	    printf("err: http req parsing ");
	}

	resp = (char **)&buffer;
	return 0;

    }else{

	printf("err: no http answer");
	return -1;
    }
}



