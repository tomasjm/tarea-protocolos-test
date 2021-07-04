//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip.h"
#include <string.h>

//MACROS
#define CLOCK_PIN       23
#define TX_PIN          22
#define RX_PIN          21

#define BYTE unsigned char

//PROTOTIPOS
void processBit(bool level);
void cb(void);

//VARIABLES GLOBALES
volatile int nbits = 0;
volatile int nbytes = 0;
bool transmissionStarted = false;
bool frameReceived = false;
BYTE bytes[50];
BYTE slipFrame[50];

int main(){
  if(wiringPiSetup() == -1){
    printf("No fue posible iniciar wiring pi\n");
    exit(1);
  }
  piHiPri(99);
  //CONFIGURA PINES DE ENTRADA SALIDA
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(CLOCK_PIN, INT_EDGE_FALLING, &cb) < 0){
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
    printf("Byte %d: 0x%x\n", i, slipFrame[i]);
  }
  int len = desempaquetaSlip(data, slipFrame);
  
  printf("\nData:\n");
  for(int i = 0; i<len; i++){
    printf("Byte %d: %d\n", i, data[i]);
  }
  
  return 0;
}

void cb(void){
  bool level = digitalRead(RX_PIN);
  processBit(level);
}

void processBit(bool level){

  //Inserta nuevo bit en byte actual
  BYTE pos = nbits;
  if(nbits>7){
    pos = 7;
    bytes[nbytes] = bytes[nbytes] >> 1;
    bytes[nbytes] &= 0x7f;
  }
  bytes[nbytes] |= level << pos;

  //Verifica si comienza transmisión
  if(!transmissionStarted && bytes[nbytes] == 0xC0){
    transmissionStarted = true;
    nbits = 0;
    nbytes++;
    // printf("Encuentra 0xc0\n");
    return;
  }

  //Actualiza contadores y banderas
  nbits++;
  if(transmissionStarted){
    if(nbits==8){
      nbits = 0;
      // printf("0x%x\n", bytes[nbytes]);
      if(bytes[nbytes] == 0xC0 && nbytes>0){
        transmissionStarted = false;
        memcpy((void*)slipFrame, (void*)bytes, nbytes+1);
        nbytes = 0;
        frameReceived = true;
        return;
      }
      nbytes++;
    }
  }
}