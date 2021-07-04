//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip.h"

//MACROS
#define clockPin 0
#define txPin 2
#define rxPin 3

#define BYTE unsigned char

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
  
  //INICIA WIRINGPI
  if(wiringPiSetup() == -1)
    exit(1);

  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(clockPin, INT_EDGE_RISING, &cbSend) < 0){
    printf("Unable to start interrupt function\n");
  }
  

  //CONFIGURA PINES DE ENTRADA SALIDA
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  //EMPAQUETA EN SLIP
  empaquetaSlip(slipArrayToSend, bytesToSend, 10);
  printf("Paquete slip: ");
  printByteArray(slipArrayToSend, 20);
  
  //TRANSMITE EL MENSAJE
  startTransmission();
  while(transmissionStartedSend)
    delay(2000);
  
  return 0;
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