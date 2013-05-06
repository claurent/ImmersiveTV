// V <type> <parameters>
// V I XXX - Instant change
// V F XXX TTT - Fade over TTT milliseconds
// V P XXX TTT - Pulse for TTT milliseconds, then turn off 
// V L XXX TTT X - Flash for TTT milliseconds X times
// V D <U/D> XX - Dim up/down by XX duty cycle 
// V Q - what's the motor say brah?

#include "mbed.h"
#include "MRF24J40.h"
#include <string>

PwmOut m1(p10);
float power;

Ticker fader;
float step; //step number in milliseconds
float target; //target rgb value

Ticker flasher;
short onflag; //is it on?
int counter = 0, maximum = 0;

Timeout clearer;

float minpower = .15;

// RF tranceiver to link with handheld.
MRF24J40 mrf(p11,p12,p13,p14,p21);


// Serial port for showing RX data.
Serial pc(USBTX,USBRX);

// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

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

void clear(){
    m1.write(0);
}

void fade(){
    if(abs(target-min(m1.read(),minpower))<=abs(step)){
        m1.write(target);
        fader.detach();
        pc.printf("Detaching!\r\n");
    }else{
        m1.write(min(m1.read(),minpower)+step);}
           
}

void flash(){
    if (onflag){
        m1.write(0);
        onflag = 0;
    }
    else{
        m1.write(target);
        counter++;
        onflag = 1;
    }
    if(counter>=maximum){
        counter = 0;
        maximum = 0;
        flasher.detach();
    }
}


int main(){
    
       
    uint8_t channel = 7;
    // Set the Channel. 0 is the default, 15 is max
    mrf.SetChannel(channel);
    
    clear();
    char *command;
    char *script;
    int setting;  
    int rxLen, value= 0, time = 0;
    int interval = 100;
 
    while(1) {
        m1.write((float) 50/100.0);
        if(pc.readable()){
        /*rxLen = rf_receive(rxBuffer, 128);
        if(rxLen > 0) {*/
            
            pc.gets(rxBuffer, 20);
            pc.printf("I got: %s\r\n", rxBuffer);
            command = strtok(rxBuffer, " ");
            if(strncmp(command, "V",1)==0) {
                command = strtok(NULL, " ");
                if(strncmp(command,"I",1)==0) {
                    value = atoi(strtok(NULL," "));
                    m1.write((float) value/100.0);
                }
                else if (strncmp(command, "F", 1)==0) {
                    target = (float)atoi(strtok(NULL," "))/100.0;
                                       
                    time = atoi(strtok(NULL," "))/interval;
                    step = (target - min(m1.read(),minpower))/time;
                    
                    pc.printf("Target: %f. Time: %i. Step size: %f\r\n", target, time, step);
                    fader.attach(&fade,(float)interval/1000.0);
                } else if (strncmp(command, "P", 1)==0) {
                    value = atoi(strtok(NULL," "));
                    m1.write((float) value/100.0);
                    time = atoi(strtok(NULL," "));
                    clearer.attach(&clear, (float)time/1000.0);
                } else if (strncmp(command, "L", 1)==0) {
                    target = (float)atoi(strtok(NULL," "))/100.0;
                    time = atoi(strtok(NULL," "));
                    counter = 0;
                    maximum = atoi(strtok(NULL," "));
                    flasher.attach(&flash,(float)time/1000.0);
                } else if (strncmp(command, "Q", 1)==0) {
                    pc.printf("Current Power: %f", m1.read());
                }
            }
           

        }
    }
}
