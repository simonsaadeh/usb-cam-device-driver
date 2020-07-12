/*
 * Userapp.c
 *
 *  Created on: 2019
 *      Author: Simon Saadeh
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dht_data.h"
#define MAGIC_VAL 'R'
#define IOCTL_GET _IO(MAGIC_VAL, 0x10)
#define IOCTL_SET _IO(MAGIC_VAL, 0x20)
#define IOCTL_STREAMON _IO(MAGIC_VAL, 0x30)
#define IOCTL_STREAMOFF _IO(MAGIC_VAL, 0x40)
#define IOCTL_GRAB _IO(MAGIC_VAL, 0x50)
#define IOCTL_PANTILT _IO(MAGIC_VAL, 0x60)
#define IOCTL_PANTILT_RESEST _IO(MAGIC_VAL, 0x70)
#define IOC_MAXNR 8

#define PANTILT_RESET  1
#define PANTILT_UP  2
#define PANTILT_DOWN  3
#define PANTILT_LEFT  4
#define PANTILT_RIGHT  5
	
#define _GET  1
#define _SET  2
#define _STREAMON  3
#define _STREAMOFF  4
#define _PANTILT  5
#define _GRAB  6
#define QUIT  7

int main(void){
	
	FILE *foutput;
	unsigned char * inBuffer;
	unsigned char * finalBuf;
  
	int mode = 0;
	int val = 0;
	
	int stay  = 1 ;
	int mySize = 0;
	int cam, ret = 0;

  	int PAN_up = 1;
  	int PAN_down = 2;
  	int PAN_left = 3;
  	int PAN_right = 4;
	
		
	struct data_t { //pour IOCTL GET ET SET
		int *data[2];
		int index;
		int value;
	};

	inBuffer = malloc((42666)* sizeof(unsigned char));
	finalBuf = malloc((42666 * 2)* sizeof(unsigned char));

	if((inBuffer == NULL) || (finalBuf == NULL)){
		return -1;
	}
	
	while (stay) {
		printf("\nVeuillez choisir une des options suivantes: \n");
		printf("1 = IOCTL SET \n");
		printf("2 = IOCTL GET \n");
		printf("3 = IOCTL STREAM ON \n");
		printf("4 = IOCTL STREAM OFF \n");
		printf("5 = IOCTL PANTILT \n");
		printf("6 = IOCTL PANTILT \n");
		printf("7 = Quitter le user panel \n");

		scanf("%i", &mode);
	
	switch (mode){
		case _GET :
				cam = open("/dev/video0", O_RDONLY);
				if(cam== -1){
					printf("file %s either does not exist or has been locked by another process\n","/d/dev/video0");
					exit(-1);
				}
		
				ret = ioctl(cam, IOCTL_GET);
				printf("ret = %d with error %d\n", ret, errno);
			break;
					
		case _SET :
				cam = open("/dev/video0", O_RDONLY);
				if(cam== -1){
					printf("file %s either does not exist or has been locked by another process\n","/d/dev/video0");
					exit(-1);
				}
				ret = ioctl(cam, IOCTL_SET);
				printf("ret = %d with error %d\n", ret, errno);
			break;
		case _STREAMON :
				cam = open("/dev/video0", O_RDONLY);
				if(cam== -1){
					printf("file %s either does not exist or has been locked by another process\n","/d/dev/video0");
					exit(-1);
				}
		
				ret = ioctl(cam, IOCTL_STREAMON);
				printf("ret = %d with error %d\n", ret, errno);
			break;
					
		case _STREAMOFF :
				cam = open("/dev/video0", O_RDONLY);
				if(cam== -1){
					printf("file %s either does not exist or has been locked by another process\n","/d/dev/video0");
					exit(-1);
				}
				ret = ioctl(cam, IOCTL_STREAMOFF);
				printf("ret = %d with error %d\n", ret, errno);
			break;
		case _PANTILT :
		
				printf("\nVeuillez choisir une des options suivantes: \n");
				printf("1 = PANTILT RESET \n");
				printf("2 = PANTILT UP \n");
				printf("3 = PANTILT DOWN ON \n");
				printf("4 = PANTILT LEFT OFF \n");
				printf("5 = PANTILT RIGHT \n");
				scanf("%i", &val);
		
				cam = open("/dev/video0", O_RDONLY);
				if(cam== -1){
					printf("file %s either does not exist or has been locked by another process\n","/d/dev/video0");
					exit(-1);
				}	
				switch (val){
					case PANTILT_RESET :
							ret = ioctl(cam, IOCTL_PANTILT_RESEST);
							printf("ret = %d with error %d\n", ret, errno);
							break;
							
					case PANTILT_UP :
							ret = ioctl(cam, IOCTL_PANTILT, &PAN_up);
							printf("ret = %d with error %d\n", ret, errno);
							break;	
					case PANTILT_DOWN :
							ret = ioctl(cam, IOCTL_PANTILT, &PAN_down);
							printf("ret = %d with error %d\n", ret, errno);
							break;
								
					case PANTILT_LEFT :
							ret = ioctl(cam, IOCTL_PANTILT, &PAN_left);
							printf("ret = %d with error %d\n", ret, errno);
							break;
					case PANTILT_RIGHT :
							ret = ioctl(cam, IOCTL_PANTILT, &PAN_right);
							printf("ret = %d with error %d\n", ret, errno);
							break;
				}
			break;				
		case _GRAB :
				cam = open("/dev/video0", O_RDONLY);
				if(cam== -1){
					printf("file %s either does not exist or has been locked by another process\n","/d/dev/video0");
					exit(-1);
				}
				
				foutput = fopen("/home/ens/AM53750/Bureau/ELE784-Lab2/allo.jpg", "wb");

				if(foutput != NULL){
					ret = ioctl(cam, IOCTL_STREAMON);
					printf("ret = %d with error %d\n", ret, errno);
					ret = ioctl(cam, IOCTL_GRAB);
					printf("ret = %d with error %d\n", ret, errno);
					mySize = read(cam, inBuffer, 42666);
					printf("mySize = %d\n", mySize);
					printf("ret = %d with error %d\n", ret, errno);
					ret = ioctl(cam, IOCTL_STREAMOFF);
					printf("ret = %d with error %d\n", ret, errno);

					memcpy (finalBuf, inBuffer, HEADERFRAME1);
					memcpy (finalBuf + HEADERFRAME1, dht_data, DHT_SIZE);
					memcpy (finalBuf + HEADERFRAME1 + DHT_SIZE, inBuffer + HEADERFRAME1, (mySize - HEADERFRAME1));
					fwrite (finalBuf, mySize + DHT_SIZE, 1, foutput);
					fclose(foutput);
			   
				}
				else
				{
					printf("No file names image.jpg");
				}
			break;
		case QUIT : //On ferme le userapp
				stay = 0; //on quite le userapp
				printf("Closing user panel\n");
			break;	
		default:
			printf("command not recognized\n");
			break;
		}		
	}

    close(cam);

    free(finalBuf);
    finalBuf = NULL;
    free(inBuffer);
    inBuffer = NULL;

    return 0;
}
