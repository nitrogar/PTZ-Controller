//
// Created by level8 on ١٧‏/٦‏/٢٠٢٠.
//
#include <linux/joystick.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "stdlib.h"
#include "main.h"
#define  REMOTE_HOST "127.0.0.1"
#define  PORT 7707
#define  UP 0
#define  DOWN 1
#define  RIGHT 2
#define  LEFT 3
struct Controller{
    char path [100];
    unsigned char keys[4];
    unsigned  char keys_type[4];
    // Negative  or Positive in case of Axial joystick  true = + , false = -;
    char keys_sign[4];
};


void logger(char * msg , char * func , int terminate){
    printf("Error : %s  in %s\n",msg,func);
    if(terminate)
        exit(1);
}
void error(char * msg , char * func){
    logger(msg , func , 1);
}


int find_controller(char * name , size_t buffer_suze){
    char controller[256];
    struct udev_device * device , * parent_dev ;
    struct dirent * dev ;
    unsigned int number_of_gamepad = 0;
    unsigned int gamepad_id ;
    DIR * dir = opendir("/dev/input");

    while((dev = readdir(dir)) != NULL){
        if(dev->d_name[0] == 'j' && dev->d_name[1] == 's') {
            sprintf(controller,"/dev/input/%s",dev->d_name);
            printf("Found Joystick : %s \n", controller);
            number_of_gamepad++;
        }
    }
    if(number_of_gamepad < 1)
        error("[!] Cant find any gamepad please connent one and make sure the device node is in /dev/input/jsX directory \n","find_controller");
    do{
        printf("[?] Choose one Joystick to be used [0-%d] :  ",number_of_gamepad - 1);
        scanf("%d",&gamepad_id);
    }while(gamepad_id > number_of_gamepad);

    snprintf(name , buffer_suze,"/dev/input/js%d" , gamepad_id);
    printf("[*] Choosing  %s\n",controller);



}
void Calibrate(struct Controller * ctrl , char * path){
    struct js_event event, prev= {};
    char Done = 0;
    int cont = 0;
    char keys[][6] = {"UP","DOWN","LEFT","RIGHT"};
    printf("[*] Calibrating  %s\n",path);
    printf("[*] Please assign %s key (press any key) \n", keys[Done]);
    int fd = open(path ,O_RDONLY );

    while(Done < 4){
        lseek(fd,0,SEEK_END);
        if(read(fd , &event , sizeof(struct js_event))> 0){

            if(event.type  != JS_EVENT_BUTTON && event.type != JS_EVENT_AXIS || (event.time - prev.time) < 1000  ) continue;
            prev = event;

            for(int i = 0 ; i < Done ; i++){
                if(event.number == ctrl->keys[i] && event.type == ctrl->keys_type[i]) {
                    if(event.type == JS_EVENT_AXIS && ((event.value > 0) && ctrl->keys_sign[i] == 1)) continue;
                    printf("[!] Key is already used , Choose another one .\n");
                    cont = 1;
                    break;
                }
            }
            if(cont) {cont = 0 ;continue;}
            printf("Assign key (%d) to %s .\n", event.number, keys[Done]);

            ctrl->keys[Done] = event.number;
            ctrl->keys_type[Done] = event.type;

            if(event.type == JS_EVENT_AXIS) ctrl->keys_sign[Done] = event.value > 0 ? 1 : 0;

        }
        Done++;
        if(Done > 3) continue;
        printf("[*] Please assign %s key (press any key) \n", keys[Done]);

    }
    close(fd);
    printf("[*] Calibration Completed .\n");
}

unsigned int parse_event(struct Controller * ctrl, struct js_event *e){
    for(int i = 0; i < 4 ; i++){
        if(e->number == ctrl->keys[i] && e->type == ctrl->keys_type[i]) {
            if(e->type == JS_EVENT_BUTTON && e->value == 1) return i;
            else if (e->type == JS_EVENT_AXIS && ctrl->keys_sign[i] == (e->value > 0)) return i;
        }
    }
    return -1;
}

int setup_connection(char * address , int port){
    int fd = 0;
    struct sockaddr_in remote;
    remote.sin_family = AF_INET;
    remote.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1" , &remote.sin_addr) < 0 )
        error("[!] Invalide remote host address\n", "setup_connection");

    fd = socket(AF_INET,SOCK_STREAM,0);

    if(fd < 0 )
        error("[!] Cant create socket \n", "setup_connection");

    if(connect(fd, (struct sockaddre*)&remote, sizeof(remote)) < 0)
        error("[!] Cant connect to remote host\n" , "setup_connection");


    return fd;

}
void send_command(char  * command , char * buffer , size_t buffer_size,int socket){
    send(socket,command, strlen(command),0);
    read(socket,buffer,buffer_size);

}

int main() {
    char gamepad_path[100] = {0};
    char receive_buffer[256] = {0};
    struct js_event Data = {};
    struct Controller ctrl = {};
    unsigned int last_time;
    int socketfd = setup_connection(REMOTE_HOST,PORT);
    find_controller(gamepad_path,sizeof(gamepad_path));
    Calibrate(&ctrl,gamepad_path);





    int fd = open(gamepad_path ,O_RDONLY );
    while(read(fd , &Data , sizeof(struct js_event)) > 0) {
        if(Data.time - last_time < 100) continue;
        switch(parse_event(&ctrl , &Data)){
            case UP:
                send_command("U",receive_buffer,sizeof(receive_buffer),socketfd);
                printf("UP \n");
                break;
            case DOWN:
                send_command("D",receive_buffer,sizeof(receive_buffer),socketfd);
                printf("DOWN\n");
                break;
            case RIGHT:
                send_command("R",receive_buffer,sizeof(receive_buffer),socketfd);
                printf("RIGHT\n");
                break;
            case LEFT:
                send_command("L",receive_buffer,sizeof(receive_buffer),socketfd);
                printf("LEFT\n");
                break;
        }
        last_time = Data.time;

    }
    close(fd);
    return 0;
}


