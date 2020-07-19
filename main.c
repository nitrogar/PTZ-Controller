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
#define  PORT 5544
#define LOMARGIN -5000
#define UPMARGIN 5000
#define  MinAxis -32768
#define MaxAxis 32767


struct Vector {
    double x;
    double y;
    double z;
};
struct Controller{
    char path [100];
    unsigned char keys[3];
    unsigned  char keys_type[3];
    // Negative  or Positive in case of Axial joystick  1 = + , 2 = -;
    char keys_sign[3];
};
enum movement {FORWARD_BACKWARD = 0,LEFT_RIGHT,UP_DOWN};


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
int between(int value , int upper , int lower){

    return value < upper && value > lower ;
}
int outside(int value , int upper , int lower){
    return value > upper || value < lower;

}
void Calibrate(struct Controller * ctrl , char * path){
    struct js_event event, prev= {};
    char Done = 0;
    int cont = 0;
    char keys[][25] = {"FORWARD BACKWARD","LEFT RIGHT","UP DOWN"};
    printf("[*] Calibrating  %s\n",path);
    printf("[*] Please assign %s key (press any key) \n", keys[Done]);
    int fd = open(path ,O_RDONLY );

    while(Done < 3){
        lseek(fd,0,SEEK_END);
        if(read(fd , &event , sizeof(struct js_event))> 0){
            if(event.type == JS_EVENT_AXIS && between(event.value , UPMARGIN,LOMARGIN)) continue;

            if(event.type  != JS_EVENT_BUTTON && event.type != JS_EVENT_AXIS || (event.time - prev.time) < 1000  ) continue;
            prev = event;


            for(int i = 0;i < Done ;i++){
                if(event.number == ctrl->keys[i] && event.type == ctrl->keys_type[i]) {
                    if((between(event.value , MaxAxis,UPMARGIN) && ctrl->keys_sign[i] == 2 )||(between(event.value , LOMARGIN,MinAxis) && ctrl->keys_sign[i] == 1 )) continue;
                    printf("[!] Key is already used as %s , %d (value %d) , Choose another one  between : %d.\n",keys[i],event.number , event.value ,between(event.value , UPMARGIN,LOMARGIN));
                    cont = 1;
                    break;
                }

            }
            if(cont) {cont = 0 ;continue;}
            printf("Assign key (%d) to %s . value %d \n", event.number, keys[Done],event.value);

            ctrl->keys[Done] = event.number;
            ctrl->keys_type[Done] = event.type;

            if(event.type == JS_EVENT_AXIS) ctrl->keys_sign[Done] = event.value > 0 ? 1 : 2;

        }
        Done++;
        //if(Done > 3) continue;
        printf("[*] Please assign %s key (press any key) \n", keys[Done]);

    }
    close(fd);
    printf("[*] Calibration Completed .\n");
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
    struct js_event e = {};
    struct Controller ctrl = {};
    unsigned int last_time;
    int time;
    double scale = 126;
    int count_x = 0,count_y = 0 , count_z = 0;
    struct Vector vec ={0};
    int socketfd = setup_connection(REMOTE_HOST,PORT);
    find_controller(gamepad_path,sizeof(gamepad_path));
    Calibrate(&ctrl,gamepad_path);


    int fd = open(gamepad_path ,O_RDONLY );
    lseek(fd,0,SEEK_END);

    while(read(fd , &e , sizeof(struct js_event)) > 0) {

            for(int i = 0; i < 3 ; i++){
                if(e.number == ctrl.keys[i] && e.type == ctrl.keys_type[i]) {

                        //send(socketfd,&vec,sizeof(vec),0);
                        switch (i) {
                        case FORWARD_BACKWARD:
                            vec.x = e.value/(double)MaxAxis;
                            break;

                        case LEFT_RIGHT:
                            vec.y = e.value/(double)MaxAxis;
                            break;

                        case UP_DOWN:
                            vec.z = e.value/(double)MaxAxis;
                            count_z++;
                            break;
                        default:
                            printf("Unkons \n");
                    }
                }
                send(socketfd,&vec,sizeof(vec),0);
                printf("FB %f \t LR %f \t UD  %f \r",vec.x,vec.y,vec.z);

                fflush(stdout);
        }
        time = e.time;

    }
    close(fd);
    return 0;
}


