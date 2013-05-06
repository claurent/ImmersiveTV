//Commands used:
//RGB - C
//Haptics - H
//Vibe Pad - V
//SCoreboard - S
//Light Tower - L

// RGB Commands
// C <type> <parameters>
// C I RRR GGG BBB - Instant change
// C F RRR GGG BBB TTT - Fade over TTT milliseconds
// C P RRR GGG BBB TTT - Pulse for TTT milliseconds, then turn off 
// C L RRR GGG BBB TTT X - Flash for TTT milliseconds X times
// C S <script> - Run a predetermined script (unimplemented)

//Light Tower commands:
// L <type> <parameters>
// L C 1/0 1/0 1/0 1/0 - Configure each pin to be either forwards or backwards
// L I 111 222 333 444 - Instant change
// L F 111 222 333 444 TTT - Fade over TTT milliseconds
// L P 111 222 333 444 TTT - Pulse for TTT milliseconds, then turn off
// L L 111 222 333 444 TTT X - Flash for TTT milliseconds X times
// L S <script> - Run a predetermined script (unimplemented)

//Haptics vest: H <top-left> <top-right> <bot-left> <bot-right> <back-left> <back-right>

#include "mbed.h"
#include "MRF24J40.h"
#include "rtos.h"

#include <string>
#include <stdio.h>
#include <stdlib.h>


// RF tranceiver to link with handheld.
MRF24J40 mrf(p11,p12,p13,p14,p21);

// Serial port for Scoreboard/RGB LEDs mBed
Serial sb(p9,p10);

// Serial port for showing RX data. Will be changed to Input Stream from USB!!!
Serial pc(USBTX,USBRX);

DigitalOut led1(LED1);

// Used for sending
char inputBuffer[128];
char previousInput[128];

// Sports Data
short sportsflag = 0; //sports data needs updating
char *team1=0;
char *prev_team1=0;
char *team2 =0;
char *prev_team2 =0;
int score1=0;
int prev_score1=0;
int score2=0;
int prev_score2=0;
char *event=0;
char *prev_event=0;

//Action data
char *movie = 0;
char *mood = 0;
char *impact = 0;
char *shot = 0;

//RGB data
short rgbflag = 0;    //rgb data needs updating
int red=0;
int green=0;
int blue=0;

//Vest map:

/**
* Receive data from the MRF24J40.
*
* @param data A pointer to a char array to hold the data
* @param maxLength The max amount of data to read.
*/
int rf_receive(char *data, uint8_t maxLength)
{
    uint8_t len = mrf.Receive((uint8_t *)data, maxLength);
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};

    if(len > 10) {
        //Remove the header and footer of the message
        for(uint8_t i = 0; i < len-2; i++) {
            if(i<8) {
                //Make sure our header is valid first
                if(data[i] != header[i])
                    return 0;
            } else {
                data[i-8] = data[i];
            }
        }

        //pc.printf("Received: %s length:%d\r\n", data, ((int)len)-10);
    }
    return ((int)len)-10;
}

/**
* Send data to another MRF24J40.
*
* @param data The string to send
* @param maxLength The length of the data to send.
*                  If you are sending a null-terminated string you can pass strlen(data)+1
*/
void rf_send(char *data, uint8_t len)
{
    //We need to prepend the message with a valid ZigBee header
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
    uint8_t *send_buf = (uint8_t *) malloc( sizeof(uint8_t) * (len+8) );

    for(uint8_t i = 0; i < len+8; i++) {
        //prepend the 8-byte header
        send_buf[i] = (i<8) ? header[i] : data[i-8];
    }
    //pc.printf("Sent: %s\r\n", send_buf+8);

    mrf.Send(send_buf, len+8);
    free(send_buf);
}
/*
void RGB(int r, int g, int b, char command, int param)
{
    if(red == r&&blue==b&&green==g)
        return;
    red = r;
    green = g;
    blue = b;
    char buffer[12];
    if(param == NULL)
        sprintf(buffer, "C %c %d %d %d\r\n",command, red, green, blue);
    else{
        if(
        sprintf(buffer, "C %c %d %d %d %i
    }    
    sb.puts(buffer);
    rgbflag = 0;
}
// light tower
void tower(int b1, int b3, int b4, char command)
{
    if(red == r&&blue==b&&green==g)
        return;
    red = r;
    green = g;
    blue = b;
    char buffer[12];
    sprintf(buffer, "C %c %d %d %d\r\n",command, red, green, blue);
    sb.puts(buffer);
    rgbflag = 0;
}
*/
int main()
{

    uint8_t channel = 7;
    // Set the Channel. 0 is the default, 15 is max
    mrf.SetChannel(channel);
    //Thread updater(update, NULL, osPriorityNormal, DEFAULT_STACK_SIZE, NULL);

    pc.baud(115200);
    //sb.baud(115200);

    pc.printf("Start----- MASTER!\r\n");

    pc.printf("Vibrator Command: 'V' AAA where AAA is target duty cycle \r\n");
    pc.printf("RGB Command: 'C' RRR GGG BBB where each are the respective duty cycles\r\n");
    pc.printf("Haptic Vest Command: 'H' XXXXXX where XXXXXX represents which motors are on (binary)\r\n");
    pc.printf("Scoreboard Command: 'S' XX where XX represent the two scores in order- 1 digit each\r\n");

    char *inputToken;
 
    while(1) {
        /*if (rgbflag = 1){
            char buffer[14];
            sprintf(buffer, "C %d %d %d\r\n", red, green ,blue);
            sb.puts(buffer);
        }*/
        // When the mBed receives an input stream... (should be every 1/30 seconds)....
        led1 = 0;
        char buffer[30];
        if (pc.readable()) {
            
            pc.gets(inputBuffer,128);
            pc.printf("I got:%s\r\n",inputBuffer);

            // Checks to see if the input stream has changed since the last frame....
            if (strcmp(previousInput,inputBuffer) != 0) {
                // The input stream has changed!
                sprintf(previousInput,"%s",inputBuffer);

                inputToken = strtok(inputBuffer, " ,");

                // Differentiate between different game modes: sports, action, etc....
                // Sports:
                if(strncmp(inputToken, "sports",6) == 0) {
                    team1 = strtok(NULL," ,"); //printf("team1 = %s\r\n", team1);
                    team2 = strtok(NULL," ,");//printf("team2 = %s\r\n", team2);
                    score1 = atoi(strtok(NULL," ,"));
                    score2 = atoi(strtok(NULL," ,"));
                    event = strtok(NULL," \n");//printf("event = %s, %d\r\n", event, strcmp(team1,event));

                    char buffer[14];
                    sprintf(buffer, "S %d\r\n", (score1 * 10 + score2));
                    pc.printf(buffer);
                    sb.puts(buffer);
                    //pc.printf("\r\nSending score to scoreboard slave via serial...");
                    //pc.printf("\r\nUpdate scores...%d", buffer);

                    // Updates the event
                    if(strcmp(event, team1) == 0) {
                        //pc.printf("Team 1 win?\r\n");
                                                
                        sb.puts("C L 0 75 75 200 4\r\n");
                        rf_send("L L 100 0 100 100 400 2\r\n", 25);
                        rf_send("V I 60\r\n",8);
                        rf_send("H 111000\r\n", 11);
                        

                    } else if(strcmp(event, team2) == 0) {
                        sb.puts("C L 0 35 75 200 4\r\n");
                        rf_send("L L 100 0 100 100 400 2\r\n", 25);
                        rf_send("V I 60\r\n",8);
                        rf_send("H 000111\r\n", 11);
                        //event1.attach(&clearRGB,.2);
                    } else if(strncmp(event, "red",5) == 0) {
                        //pc.printf("Start?\r\n");
                        sb.puts("C L 100 0 0 200 4\r\n");
                        rf_send("V I 60\r\n",8);
                        //event2.attach(&clearVib, .2);
                    } else if(strncmp(event, "end",3) == 0) {

                    } else if(strncmp(event, "nil",3) == 0) {
                        sb.puts("C F 20 100 30 999\r\n");
                        rf_send("V I 50\r\n",8);

                    }
                } else if(strncmp(inputToken, "action",6) == 0) {
                    //action BoondockSaints <mood> <impact> <shot>
                    //moods: bright, indoor, dark, action
                    //impact: left-right, back, nil
                    //shot: right-top, right-bot, left-top, left-bot, nil
                    movie = strtok(NULL, " ");
                    mood = strtok(NULL, " ");
                    impact = strtok(NULL, " ");
                    shot = strtok(NULL, " ");
                    
                    if (strncmp(mood,"bright",6)==0){
                        sb.puts("C I 100 75 75\r\n");
                        //use light tower?
                        rf_send("L F 100 100 100 100 500\r\n",25);
                        if(strncmp(shot, "head",4)){
                            rf_send("H 111111\r\n",10);
                            //vibration pulse here for 100 ms?!
                        }
                    } else if(strncmp(mood,"indoor",6)==0){
                        sb.puts("C F 0 75 75 999\r\n");
                        if(strncmp(impact,"back",4)== 0){
                            rf_send("H 000011\r\n",10);
                    } else if(strncmp(mood,"dark",4)==0){
                        sb.puts("C I 0 0 0\r\n");
                        rf_send("L I 0 0 0 0\r\n",13);
                    } else if(strncmp(mood,"action",6)==0){
                        sb.puts("C L 100 0 0 2000 2\r\n");
                    }
                    } else if(strncmp(mood,"nil",3)==0){
                        sb.puts("C I 0 0 0\r\n");
                        rf_send("L I 0 0 0\r\n",11);
                        rf_send("V 0\r\n",5);
                    }
                        /*if(strncmp(shot,"right-top",9)==0)
                         rf_send("H 010000\r\n",10);
                        else if(strncmp(shot,"right-bot",9)==0)
                            rf_send("H 000100\r\n",10);
                        else if(strncmp(shot,"left-top",8)==0)
                            rf_send("H 100000\r\n",10);
                        else if(strncmp(shot,"left-bot",8)==0)
                            rf_send("H 001000\r\n",10);
                        else if(strncmp(impact,"front",5)==0){
                            rf_send("H 111100\r\n",10);        */
                    
                        if(strncmp(impact,"right-top",9)==0)
                         rf_send("H 010000\r\n",10);
                        else if(strncmp(impact,"right-bot",9)==0)
                            rf_send("H 000100\r\n",10);
                        else if(strncmp(impact,"left-top",8)==0)
                            rf_send("H 100000\r\n",10);
                        else if(strncmp(impact,"left-bot",8)==0)
                            rf_send("H 001000\r\n",10);
                        else if(strncmp(impact,"front",5)==0){
                            rf_send("H 111100\r\n",10);        
                
                /*
                if(strncmp(inputToken, "V",1)==0){
                  targduty=atoi(strtok(NULL," ,"));
                  ramptime=atoi(strtok(NULL," ,"));
                }*/
            }
            // Sends the input stream out.... WILL BE REMOVED!
            //rf_send(inputBuffer, strlen(inputBuffer) + 1);
        }
    }
}}}
