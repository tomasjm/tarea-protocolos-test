//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip.h"
#include <string.h>

//MACROS
#define clockPin1      0
#define clockPin       23
#define txPin          22
#define rxPin          21

#define BYTE unsigned char

//PROTOTIPOS
void processBit(bool level);
void cbReceive(void);

//VARIABLES GLOBALES
volatile int nbitsReceived = 0;
volatile int nbytesReceived = 0;
bool transmissionStartedReceive = false;
bool frameReceived = false;
BYTE bytesReceived[50];
BYTE slipArrayReceived[50];


//envia 

//DECLARACION DE PROTOTIPOS
void cbSend(void);
void startTransmission();
void printByteArray(BYTE* arr, int len);

//VARIABLES GLOBALES
volatile int nbitsSend = 0;
BYTE bytesToSend[10] = {1,2,3,4,5,6,7,8,9,10};
BYTE slipArrayToSend[20];
volatile int nbytesSend = 0;
BYTE len = 10;
int nones = 0;
bool transmissionStartedSend = false;
int endCount = 0;

int main(){
  if(wiringPiSetup() == -1){
    printf("No fue posible iniciar wiring pi\n");
    exit(1);
  }
  piHiPri(99);
  //CONFIGURA PINES DE ENTRADA SALIDA
  
  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(clockPin, INT_EDGE_BOTH, &cbReceive) < 0){
    printf("Unable to start interrupt function\n");
  }
  if(wiringPiISR(clockPin, INT_EDGE_BOTH, &cbSend) < 0){
    printf("Unable to start interrupt function\n");
  }
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  for(int i = 0; i<50; i++){
    bytesReceived[i] = 0;
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

void cbReceive(void){
  bool level = digitalRead(rxPin);
  processBit(level);
}

void processBit(bool level){

  //Inserta nuevo bit en byte actual
  BYTE pos = nbitsReceived;
  if(nbitsReceived>7){
    pos = 7;
    bytesReceived[nbytesReceived] = bytesReceived[nbytesReceived] >> 1;
    bytesReceived[nbytesReceived] &= 0x7f;
  }
  bytesReceived[nbytesReceived] |= level << pos;

  //Verifica si comienza transmisión
  if(!transmissionStartedReceive && bytesReceived[nbytesReceived] == 0xC0){
    transmissionStartedReceive = true;
    nbitsReceived = 0;
    nbytesReceived++;
    // printf("Encuentra 0xc0\n");
    return;
  }

  //Actualiza contadores y banderas
  nbitsReceived++;
  if(transmissionStartedReceive){
    if(nbitsReceived==8){
      nbitsReceived = 0;
      // printf("0x%x\n", bytesReceived[nbytesReceived]);
      if(bytesReceived[nbytesReceived] == 0xC0 && nbytesReceived>0){
        transmissionStartedReceive = false;
        memcpy((void*)slipArrayReceived, (void*)bytesReceived, nbytesReceived+1);
        nbytesReceived = 0;
        frameReceived = true;
        return;
      }
      nbytesReceived++;
    }
  }
}

void cbSend(void){
  if(transmissionStartedSend){
    if(endCount == 0 && slipArrayToSend[nbytesSend] != 0xC0){
      nbytesSend++;
      return;
    }

    //Escribe en el pin TX
    digitalWrite(txPin, (slipArrayToSend[nbytesSend] >> nbitsSend) & 0x01); //Bit de dato

    //Actualiza contador de bits
    nbitsSend++;

    //Actualiza contador de bytes
    if(nbitsSend == 8){
      nbitsSend = 0;
      endCount += slipArrayToSend[nbytesSend] == 0xC0;
      //Finaliza la comunicación
      if(slipArrayToSend[nbytesSend] == 0xC0 && endCount>1){
        endCount = 0;
        nbytesSend = 0;
        transmissionStartedSend = false;
        return;
      }
      nbytesSend++;      
    }
  }else{
    //Canal en reposo
    digitalWrite(txPin, 1);
  }
}

void startTransmission(){
  transmissionStartedSend = true;
}

void printByteArray(BYTE* arr, int len){
  for(int i = 0; i<len; i++){
    printf("0x%x ", arr[i]);
  }
  printf("\n");
}