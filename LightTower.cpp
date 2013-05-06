//Talk to this guy with L
//Light Tower commands:
// L <type> <parameters>
// L C 1/0 1/0 1/0 1/0 - Configure each pin to be either forwards or backwards
// L I 111 222 333 444 - Instant change
// L F 111 222 333 444 TTT - Fade over TTT milliseconds
// L P 111 222 333 444 TTT - Pulse for TTT milliseconds, then turn off
// L L 111 222 333 444 TTT X - Flash for TTT milliseconds X times
// L S <script> - Run a predetermined script
// L D <U/D> XX - Dim up/down by XX duty cycle
// 0 - high, 100% - low

#include "mbed.h"
#include "MRF24J40.h"

#include <string>

PwmOut b1(p5);
DigitalOut u1(p16);
DigitalOut d1(p17);
PwmOut b2(p6);
DigitalOut u2(p18);
DigitalOut d2(p19);
PwmOut b3(p26);
DigitalOut u3(p29);
DigitalOut d3(p30);
PwmOut b4(p25);
DigitalOut u4(p23);
DigitalOut d4(p24);
MRF24J40 mrf(p11,p12,p13,p14,p21);

char txBuffer[128];
char rxBuffer[30];

Timeout clear;

Ticker pulse;

Ticker fader;
float step1, step2, step3, step4; //step number in milliseconds
float target1, target2, target3, target4; //target values

Ticker flash;
short onflag; //is it on?
int counter = 0, maximum = 0;


Ticker scriptRunner; //300 ms per step, 10 step limit, EOS delimited by 1
float pattern[10][4];
float celeb[10][4] = {
{.90,0,0,0},
{0,.90,0,0},
{0,0,.9,0},
{0,0,0,.9},
{.90,0,0,0},
{0,.9,0,0},
{0,0,.9,0},
{0,0,0,.9},
{.90,0,0,0},
{0,.90,0,0}};


// Serial port for PC.
Serial pc(USBTX,USBRX);



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

void clearLights()
{
    b1.write(0);
    b2.write(0);
    b3.write(0);
    b4.write(0);
}

void report(){
    printf("Status: %f %f %f %f\r\n", b1.read(), b2.read(), b3.read(), b4.read());
}

void fadeLight()
{
    short flag1=0, flag2=0, flag3=0, flag4 = 0; //reached target?
    //pc.printf("Max: %f, Min: %f\r\n",max(target4,b4.read()),min(target4,b4.read()));
    if(abs(target1-b1.read())<=abs(step1)) {
        
        b1.write(target1);
        flag1 = 1;
    } else {
        b1.write(b1.read()+step1);
    }
    
    if(abs(target2-b2.read())<=abs(step2)) {
        b2.write(target2);
        flag2 = 1;
    } else {
        b2.write(b2.read()+step2);
    }
    
    if(abs(target3-b3.read())<=abs(step3)) {
        b3.write(target3);
        flag3 = 1;
    } else {
        b3.write(b3.read()+step3);
    }
    
    if(abs(target4-b4.read())<=abs(step4)) {
        b4.write(target4);
        flag4 = 1;
    } else {
        b4.write(b4.read()+step4);
    }

    if(flag1&&flag2&&flag3&&flag4){ //we've reached the target
        pc.printf("Trying to detach!");
        fader.detach();
    }
}

void runScript(){
    if(counter >= maximum) {
        counter = 0;
        maximum = 0;
        scriptRunner.detach();
        clearLights();
        return;
    }
    else{
        b1.write(pattern[counter][0]);
        b2.write(pattern[counter][1]);
        b3.write(pattern[counter][2]);
        b4.write(pattern[counter][3]);
        counter++;
    }
    
}

void flashLight()
{
    if(counter>=maximum) {
        counter = 0;
        maximum = 0;
        flash.detach();
        clearLights();
        return;
    }
    if (onflag) {
        clearLights();
        onflag = 0;
    } else {
        b1.write(target1);
        b2.write(target2);
        b3.write(target3);
        b4.write(target4);
        counter++;
        onflag = 1;
    }
}

// Current score
int score;

char buffer[128];

int main()
{
    uint8_t channel = 7;
    // Set the Channel. 0 is the default, 15 is max
    mrf.SetChannel(channel);
    pc.printf("Start----- Light Tower!\r\n");
    
    u1 = u2 = u3 = u4 = 1;
    d1 = d2 = d3 = d4 = 0;
    clearLights();
    char *command;
    char *script;
    int setting;  
    int rxLen, value= 0, time = 0;
    int interval = 100;
    pulse.attach(&report, 3);
    while(1) {
        
        if(pc.readable()){
        /*rxLen = rf_receive(rxBuffer, 128);
        if(rxLen > 0) {*/
            
            pc.gets(rxBuffer, 30);
            pc.printf("I got: %s\r\n", rxBuffer);
            
            command = strtok(rxBuffer, " ");
            if(strncmp(command, "L",1)==0) {
                command = strtok(NULL, " ");
                if(strncmp(command,"I",1)==0) {
                    value = atoi(strtok(NULL," "));
                    b1.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    b2.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    b3.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    b4.write((float) value/100.0);
                    pc.printf("Current setting: %f %f %f %f\r\n", b1.read(),b2.read(),b3.read(),b4.read());
                }
                else if(strncmp(command,"C", 1)==0){
                    setting = atoi(strtok(NULL," "));
                    if(setting){ u1 = 1; d1 = 0;}
                    else{ u1 = 0; d1 = 1; }
                    
                    setting = atoi(strtok(NULL," "));
                    if(setting){ u2 = 1; d2 = 0;}
                    else{ u2 = 0; d2 = 1; }
                    
                    setting = atoi(strtok(NULL," "));
                    if(setting){ u3 = 1; d3 = 0;}
                    else{ u3 = 0; d3 = 1; }
                    
                    setting = atoi(strtok(NULL," "));
                    if(setting){ u4 = 1; d4 = 0;}
                    else{ u4 = 0; d4 = 1; }    
                    pc.printf("Settings set!\r\n");
                }
                else if (strncmp(command, "F", 1)==0) {
                    target1 = (float)atoi(strtok(NULL," "))/100.0;
                    target2 = (float)atoi(strtok(NULL," "))/100.0;
                    target3 = (float)atoi(strtok(NULL," "))/100.0;
                    target4 = (float)atoi(strtok(NULL," "))/100.0;
                    
                    time = atoi(strtok(NULL," "))/interval;
                    step1 = (target1 - b1.read())/time;
                    step2 = (target2 - b2.read())/time;
                    step3 = (target3 - b3.read())/time;
                    step4 = (target4 - b4.read())/time;
                    pc.printf("Target: %f. Time: %i. Step size: %f\r\n", target4, time, step4);
                    fader.attach(&fadeLight,(float)interval/1000.0);
                } else if (strncmp(command, "P", 1)==0) {
                    b1.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    b2.write((float) value/100.0);
                    value = atoi(strtok(NULL," "));
                    b3.write((float) value/100.0);
                    time = atoi(strtok(NULL," "));
                    b4.write((float) value/100.0);
                    time = atoi(strtok(NULL," "));
                    clear.attach(&clearLights, (float)time/1000.0);
                } else if (strncmp(command, "L", 1)==0) {
                    target1 = (float)atoi(strtok(NULL," "))/100.0;
                    target2 = (float)atoi(strtok(NULL," "))/100.0;
                    target3 = (float)atoi(strtok(NULL," "))/100.0;
                    target4 = (float)atoi(strtok(NULL," "))/100.0;
                    time = atoi(strtok(NULL," "));
                    counter = 0;
                    maximum = atoi(strtok(NULL," "));
                    flash.attach(&flashLight,(float)time/1000.0);
                } else if (strncmp(command, "S", 1)==0) {
                    script = strtok(NULL," ");
                    if(strncmp(script, "celeb", 5) == 0){
                        memcpy(pattern, celeb, sizeof(float) * 4 * 10);
                        maximum = 10; counter = 0;
                        for(int i = 0; i < 10; i++){
                            if (celeb[i][0] == 1)
                                maximum = i;
                        }
                        scriptRunner.attach(&runScript,.5);
                        
                    }
                } else if (strncmp(command, "D", 1)==0) {
                }
            }
           

        }
    }
}
