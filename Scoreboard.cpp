#include "mbed.h"
#include "MRF24J40.h"

#include <string>

BusOut anodesRight(p33,p34,p35,p36);
BusOut anodesLeft(p23,p24,p25,p26);

DigitalOut switchLeft(p29);
DigitalOut switchRight(p30);

PwmOut red_pin(p6);
PwmOut green_pin(p20);
PwmOut blue_pin(p5);

Timeout clear;

Ticker fader;
float rstep, bstep, gstep; //step number in milliseconds
float rtarget, btarget, gtarget; //target rgb value

Ticker flash;
short onflag; //is it on?
int counter = 0, maximum = 0;

// RGB Commands
// C <type> <parameters>
// C I RRR GGG BBB - Instant change
// C F RRR GGG BBB TTT - Fade over TTT milliseconds
// C P RRR GGG BBB TTT - Pulse for TTT milliseconds, then turn off 
// C L RRR GGG BBB TTT X - Flash for TTT milliseconds X times
// C S <script> - Run a predetermined script
// C D <U/D> XX - Dim up/down by XX duty cycle 

// Score Commands
// S E <1/0>
// S XX

// Serial port for communicating with master mBed
Serial master(p9,p10);

// Serial port for PC.
Serial pc(USBTX,USBRX);

// Current score
int score;

char buffer[128];

void updateScore() {
    anodesRight = score % 10;
    anodesLeft = (score / 10) % 10;
    }

void clearRGB(){
    red_pin.write(0);
    green_pin.write(0);
    blue_pin.write(0);
}

void fadeRGB(){
    short rflag=0, gflag=0, bflag=0; //reached target?
    if(abs(rtarget-red_pin.read())<=abs(rstep)){
        red_pin.write(rtarget);
        rflag = 1;
    }else{
        red_pin.write(red_pin.read()+rstep);}
        
    if(abs(gtarget-green_pin.read())<=abs(gstep)){
        green_pin.write(gtarget);
        gflag = 1;
    }else{
        green_pin.write(green_pin.read()+gstep);}
        
    if(abs(btarget-blue_pin.read())<=abs(bstep)){
        blue_pin.write(btarget);
        bflag = 1;
    }else{
        blue_pin.write(blue_pin.read()+bstep);}
    
    if(rflag&&gflag&&bflag){ //we've reached the target
        fader.detach();
        pc.printf("Detaching!\r\n");
    }
        
}

void flashRGB(){
    if (onflag){
        red_pin.write(0);
        green_pin.write(0);
        blue_pin.write(0);
        onflag = 0;
    }
    else{
        red_pin.write(rtarget);
        green_pin.write(gtarget);
        blue_pin.write(btarget);
        counter++;
        onflag = 1;
    }
    if(counter>=maximum){
        counter = 0;
        maximum = 0;
        flash.detach();
    }
}

int main()
{   
    int value=0, time=0;
    int interval = 100; //default step interval
    score = 00;
    updateScore();
    //master.baud(115200);
    
    switchLeft = switchRight = 1;
    
    red_pin.write(0.0);
    green_pin.write(0.0);
    blue_pin.write(0.0);
    
    pc.printf("Start----- Scoreboard+RGB!\r\n");
    char *inputToken;
    char *command;
    char *noScore;
    
    while(1) {
        if(master.readable()) {
            
            master.gets(buffer,128);
            pc.printf("I got: %s\r\n",buffer);
            inputToken = strtok(buffer, " ");
            
                
            // Differentiate between scoreboard and RGB commands
            if(strncmp(inputToken,"S",1) == 0) {
                // updates the score
                noScore = strtok(NULL," ,");
                score = atoi(noScore);
                switchLeft = switchRight = 1;
                if (strncmp(noScore, "E", 1)){
                    switchLeft = switchRight = atoi(strtok(NULL," "));
                } else{
                updateScore();
                }
            }
            else if(strncmp(inputToken,"C",1) == 0){
                command = strtok(NULL," ");
                if(strncmp(command,"I",1)==0){
                    red_pin.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    green_pin.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    blue_pin.write((float) value/100.0);
                    pc.printf("Current setting: %f %f %f\r\n", red_pin.read(),green_pin.read(),blue_pin.read());                    
                } else if (strncmp(command, "F", 1)==0){
                    rtarget = (float)atoi(strtok(NULL," "))/100.0;
                    gtarget = (float)atoi(strtok(NULL," "))/100.0;
                    btarget = (float)atoi(strtok(NULL," "))/100.0;
                    time = atoi(strtok(NULL," "))/interval;
                    rstep = (rtarget - red_pin.read())/time;
                    gstep = (gtarget - green_pin.read())/time;
                    bstep = (btarget - blue_pin.read())/time;
                    fader.attach(&fadeRGB,(float)interval/1000.0);
                } else if (strncmp(command, "P", 1)==0){
                    red_pin.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    green_pin.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    blue_pin.write((float) value/100.0);
                    time = atoi(strtok(NULL," "));
                    clear.attach(&clearRGB, (float)time/1000.0);
                } else if (strncmp(command, "L", 1)==0){
                    rtarget = (float)atoi(strtok(NULL," "))/100.0;
                    gtarget = (float)atoi(strtok(NULL," "))/100.0;
                    btarget = (float)atoi(strtok(NULL," "))/100.0;
                    time = atoi(strtok(NULL," "));
                    maximum = atoi(strtok(NULL," "));
                    flash.attach(&flashRGB, (float)time/1000.0);
                } else if (strncmp(command, "S", 1)==0){
                } else if (strncmp(command, "D", 1)==0){
                }
                /*value = atoi(strtok(NULL," "));
                red_pin.write((float) value/100.0);
                value = atoi(strtok(NULL," "));
                green_pin.write((float) value/100.0);
                value = atoi(strtok(NULL," "));
                blue_pin.write((float) value/100.0);
                pc.printf("Current setting: %f %f %f\r\n", red_pin.read(),green_pin.read(),blue_pin.read());*/
            
            
            }
            
        }
        
        
        /*rxLen = rf_receive(rxBuffer, 128);
        if(rxLen > 0) {
            pc.printf("Received: %s\r\n", rxBuffer);

            command = strtok(rxBuffer, " ");
            if(strncmp(command, "S",1)==0) {
                score = atoi(strtok(NULL," "));

            }
        //cathodes = 0x03;
                anodesRight = score % 10;
                anodesLeft = (score / 10) % 10;
        }*/
    }
}
