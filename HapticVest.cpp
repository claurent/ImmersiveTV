#include "mbed.h"
#include "MRF24J40.h"

#include <string>

Timeout to1;
Timeout to2;
Timeout to3;
Timeout to4;
Timeout to5;
Timeout to6;

DigitalOut motor1(p25);
DigitalOut motor2(p23);
DigitalOut motor3(p26);
DigitalOut motor4(p29);
DigitalOut motor5(p30);
DigitalOut motor6(p24);
DigitalOut led1(LED1);

int motorStatus;

// Serial port for showing RX data.
Serial pc(USBTX,USBRX);


// RF tranceiver to link with handheld.
MRF24J40 mrf(p11,p12,p13,p14,p21);

//Haptics vest: H <top-left> <top-right> <bot-left> <bot-right> <back-left> <back-right>

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


// Timeout for vibration
void shot1() {
     motor1 = !motor1;
}
void shot2() {
     motor2 = !motor2;
}
void shot3() {
     motor3 = !motor3;
}
void shot4() {
     motor4 = !motor4;
}
void shot5() {
     motor5 = !motor5;
}
void shot6() {
     motor6 = !motor6;
}

void updateMotors() {
    if(motor1) to1.attach(&shot1,.2);
    if(motor2) to2.attach(&shot2,.2);
    if(motor3) to3.attach(&shot3,.2);
    if(motor4) to4.attach(&shot4,.2);
    if(motor5) to5.attach(&shot5,.2);
    if(motor6) to6.attach(&shot6,.2);
}

int main() {
    motor1 = 0;
    motor2 = 0;
    motor3 = 0;
    motor4 = 0;
    motor5 = 0;
    motor6 = 0;
    
    uint8_t channel = 7;
    // Set the Channel. 0 is the default, 15 is max
    mrf.SetChannel(channel);
    
    char *command;
    
    pc.printf("Start----- Haptic Vest!\r\n");
    
    
    while(1) {
        rxLen = rf_receive(rxBuffer, 128);
        if(rxLen > 0) {
            pc.printf("Received: %s\r\n", rxBuffer);
            led1 = 1;
            
            command = strtok(rxBuffer, " ");
            if(strncmp(command, "H",1) == 0){
                motorStatus = atoi(strtok(NULL," "));
                
                motor6 = motorStatus % 10;
                motor5 = (motorStatus / 10) % 10;
                motor4 = (motorStatus / 100) % 10;
                motor3 = (motorStatus / 1000) % 10;
                motor2 = (motorStatus / 10000) % 10;
                motor1 = (motorStatus / 100000) % 10;
                
                updateMotors();
                
            }
            
            
        }  
       
    }
}
