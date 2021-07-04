//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip.h"
#include <string.h>

//MACROS
#define CLOCK_PIN_RECEIVE       23
#define TX_PIN_RECEIVE          22
#define RX_PIN_RECEIVE          21

#define BYTE unsigned char

//PROTOTIPOS
void processBit(bool level);
void cbReceive(void);

//VARIABLES GLOBALES
volatile int nbitsReceive = 0;
volatile int nbytesReceive = 0;
bool transmissionStartedReceive = false;
bool frameReceived = false;
BYTE slipFrame[50];

int main(){
  if(wiringPiSetup() == -1){
    printf("No fue posible iniciar wiring pi\n");
    exit(1);
  }
  piHiPri(99);
  //CONFIGURA PINES DE ENTRADA SALIDA
  pinMode(RX_PIN_RECEIVE, INPUT);
  pinMode(TX_PIN_RECEIVE, OUTPUT);

  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(CLOCK_PIN_RECEIVE, INT_EDGE_FALLING, &cbReceive) < 0){
    printf("Unable to start interrupt function\n");
  }


  printf("Delay\n");
  while(!frameReceived)
    delay(300);
  
  printf("Frame slip recibido!\n");
  BYTE data[50];
  for(int i = 0; i<50; i++){
    printf("Byte %d: 0x%x\n", i, slipFrame[i]);
  }
  int len = desempaquetaSlip(data, slipFrame);
  
  printf("\nData:\n");
  for(int i = 0; i<len; i++){
    printf("Byte %d: %d\n", i, data[i]);
  }
  
  return 0;
}

void cbReceive(void){
  bool level = digitalRead(RX_PIN_RECEIVE);
  processBit(level);
}

void processBit(bool level){

  //Inserta nuevo bit en byte actual
  BYTE pos = nbitsReceive;
  if(nbitsReceive>7){
    pos = 7;
    bytes[nbytesReceive] = bytes[nbytesReceive] >> 1;
    bytes[nbytesReceive] &= 0x7f;
  }
  bytes[nbytesReceive] |= level << pos;

  //Verifica si comienza transmisión
  if(!transmissionStartedReceive && bytes[nbytesReceive] == 0xC0){
    transmissionStartedReceive = true;
    nbitsReceive = 0;
    nbytesReceive++;
    // printf("Encuentra 0xc0\n");
    return;
  }

  //Actualiza contadores y banderas
  nbitsReceive++;
  if(transmissionStartedReceive){
    if(nbitsReceive==8){
      nbitsReceive = 0;
      // printf("0x%x\n", bytes[nbytesReceive]);
      if(bytes[nbytesReceive] == 0xC0 && nbytesReceive>0){
        transmissionStartedReceive = false;
        memcpy((void*)slipFrame, (void*)bytes, nbytesReceive+1);
        nbytesReceive = 0;
        frameReceived = true;
        return;
      }
      nbytesReceive++;
    }
  }
}