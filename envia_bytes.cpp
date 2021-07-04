//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip.h"

//MACROS
#define CLOCK_PIN 0
#define TX_PIN 2
#define RX_PIN 3

#define BYTE unsigned char

//DECLARACION DE PROTOTIPOS
void cb(void);
void startTransmission();
void printByteArray(BYTE* arr, int len);

//VARIABLES GLOBALES
volatile int nbits = 0;
BYTE bytes[10] = {1,2,3,4,5,6,7,8,9,10};
BYTE slipFrame[20];
volatile int nbytes = 0;
BYTE len = 10;
int nones = 0;
bool transmissionStarted = false;
int endCount = 0;

int main(){
  
  //INICIA WIRINGPI
  if(wiringPiSetup() == -1)
    exit(1);

  //CONFIGURA INTERRUPCION PIN CLOCK (PUENTEADO A PIN PWM)
  if(wiringPiISR(CLOCK_PIN, INT_EDGE_RISING, &cb) < 0){
    printf("Unable to start interrupt function\n");
  }

  //CONFIGURA PINES DE ENTRADA SALIDA
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  //EMPAQUETA EN SLIP
  empaquetaSlip(slipFrame, bytes, 10);
  printf("Paquete slip: ");
  printByteArray(slipFrame, 20);
  
  //TRANSMITE EL MENSAJE
  startTransmission();
  while(transmissionStarted)
    delay(2000);
  
  return 0;
}

void cb(void){
  if(transmissionStarted){
    if(endCount == 0 && slipFrame[nbytes] != 0xC0){
      nbytes++;
      return;
    }

    //Escribe en el pin TX
    digitalWrite(TX_PIN, (slipFrame[nbytes] >> nbits) & 0x01); //Bit de dato

    //Actualiza contador de bits
    nbits++;

    //Actualiza contador de bytes
    if(nbits == 8){
      nbits = 0;
      endCount += slipFrame[nbytes] == 0xC0;
      //Finaliza la comunicación
      if(slipFrame[nbytes] == 0xC0 && endCount>1){
        endCount = 0;
        nbytes = 0;
        transmissionStarted = false;
        return;
      }
      nbytes++;      
    }
  }else{
    //Canal en reposo
    digitalWrite(TX_PIN, 1);
  }
}

void startTransmission(){
  transmissionStarted = true;
}

void printByteArray(BYTE* arr, int len){
  for(int i = 0; i<len; i++){
    printf("0x%x ", arr[i]);
  }
  printf("\n");
}