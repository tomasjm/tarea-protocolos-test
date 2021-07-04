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
void cb(void);

//VARIABLES GLOBALES
volatile int nbitsReceived = 0;
volatile int nbytesReceived = 0;
bool transmissionStarted = false;
bool frameReceived = false;
BYTE bytesReceived[50];
BYTE slipArrayReceived[50];

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
  if(wiringPiISR(CLOCK_PIN_RECEIVE, INT_EDGE_FALLING, &cb) < 0){
    printf("Unable to start interrupt function\n");
  }

  for(int i = 0; i<50; i++){
    bytes[i] = 0;
  }

  printf("Delay\n");
  while(!frameReceived)
    delay(300);
  
  printf("Frame slip recibido!\n");
  BYTE data[50];
  for(int i = 0; i<50; i++){
    printf("Byte %d: 0x%x\n", i, slipArrayReceived[i]);
  }
  int len = desempaquetaSlip(data, slipArrayReceived);
  
  printf("\nData:\n");
  for(int i = 0; i<len; i++){
    printf("Byte %d: %d\n", i, data[i]);
  }
  
  return 0;
}

void cb(void){
  bool level = digitalRead(RX_PIN_RECEIVE);
  processBit(level);
}

void processBit(bool level){

  //Inserta nuevo bit en byte actual
  BYTE pos = nbitsReceived;
  if(nbitsReceived>7){
    pos = 7;
    bytes[nbytesReceived] = bytes[nbytesReceived] >> 1;
    bytes[nbytesReceived] &= 0x7f;
  }
  bytes[nbytesReceived] |= level << pos;

  //Verifica si comienza transmisión
  if(!transmissionStarted && bytes[nbytesReceived] == 0xC0){
    transmissionStarted = true;
    nbitsReceived = 0;
    nbytesReceived++;
    // printf("Encuentra 0xc0\n");
    return;
  }

  //Actualiza contadores y banderas
  nbitsReceived++;
  if(transmissionStarted){
    if(nbitsReceived==8){
      nbitsReceived = 0;
      // printf("0x%x\n", bytes[nbytesReceived]);
      if(bytes[nbytesReceived] == 0xC0 && nbytesReceived>0){
        transmissionStarted = false;
        memcpy((void*)slipArrayReceived, (void*)bytes, nbytesReceived+1);
        nbytesReceived = 0;
        frameReceived = true;
        return;
      }
      nbytesReceived++;
    }
  }
}