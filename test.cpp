//INCLUSIONES
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include "slip.h"
#include <string.h>

//MACROS
#define CLOCK_PIN_SEND 0
#define TX_PIN_SEND 2
#define RX_PIN_SEND 3

#define CLOCK_PIN_RECEIVE 23
#define TX_PIN_RECEIVE 22
#define RX_PIN_RECEIVE 21

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

//DECLARACION DE PROTOTIPOS
void cbSend(void);
void startTransmission();
void printByteArray(BYTE *arr, int len);

//VARIABLES GLOBALES
volatile int nbitsSend = 0;
BYTE bytesToSend[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
BYTE slipArrayToSend[20];
volatile int nbytesSend = 0;
BYTE len = 10;
int nones = 0;
bool transmissionStartedSend = false;
int endCount = 0;

int main(int argc, char *args[])
{
  //INICIA WIRINGPI
  if (wiringPiSetup() == -1)
    exit(1);
  //wiringPiISR(CLOCK_PIN_SEND, INT_EDGE_RISING, &cbSend);
  wiringPiISR(CLOCK_PIN_RECEIVE, INT_EDGE_FALLING, &cbReceive);
  if (argc > 1 && atoi(args[1]) == 1)
  {
  }
  else
  {
    piHiPri(99);
    for (int i = 0; i < 50; i++)
    {
      bytesReceived[i] = 0;
    }

    printf("Delay\n");
    while (!frameReceived)
      delay(300);

    printf("Frame slip recibido!\n");
    BYTE data[50];
    for (int i = 0; i < 50; i++)
    {
      printf("Byte %d: 0x%x\n", i, slipArrayReceived[i]);
    }
    int len = desempaquetaSlip(data, slipArrayReceived);

    printf("\nData:\n");
    for (int i = 0; i < len; i++)
    {
      printf("Byte %d: %d\n", i, data[i]);
    }
  }

  return 0;
}

void cbReceive(void)
{
  bool level = digitalRead(RX_PIN_RECEIVE);
  processBit(level);
}

void processBit(bool level)
{

  //Inserta nuevo bit en byte actual
  BYTE pos = nbitsReceived;
  if (nbitsReceived > 7)
  {
    pos = 7;
    bytesReceived[nbytesReceived] = bytesReceived[nbytesReceived] >> 1;
    bytesReceived[nbytesReceived] &= 0x7f;
  }
  bytesReceived[nbytesReceived] |= level << pos;

  //Verifica si comienza transmisión
  if (!transmissionStartedReceive && bytesReceived[nbytesReceived] == 0xC0)
  {
    transmissionStartedReceive = true;
    nbitsReceived = 0;
    nbytesReceived++;
    // printf("Encuentra 0xc0\n");
    return;
  }

  //Actualiza contadores y banderas
  nbitsReceived++;
  if (transmissionStartedReceive)
  {
    if (nbitsReceived == 8)
    {
      nbitsReceived = 0;
      // printf("0x%x\n", bytesReceived[nbytesReceived]);
      if (bytesReceived[nbytesReceived] == 0xC0 && nbytesReceived > 0)
      {
        transmissionStartedReceive = false;
        memcpy((void *)slipArrayReceived, (void *)bytesReceived, nbytesReceived + 1);
        nbytesReceived = 0;
        frameReceived = true;
        return;
      }
      nbytesReceived++;
    }
  }
}
void cbSend(void)
{
  if (transmissionStartedSend)
  {
    if (endCount == 0 && slipArrayToSend[nbytesSend] != 0xC0)
    {
      nbytesSend++;
      return;
    }

    //Escribe en el pin TX
    digitalWrite(TX_PIN_SEND, (slipArrayToSend[nbytesSend] >> nbitsSend) & 0x01); //Bit de dato

    //Actualiza contador de bits
    nbitsSend++;

    //Actualiza contador de bytes
    if (nbitsSend == 8)
    {
      nbitsSend = 0;
      endCount += slipArrayToSend[nbytesSend] == 0xC0;
      //Finaliza la comunicación
      if (slipArrayToSend[nbytesSend] == 0xC0 && endCount > 1)
      {
        endCount = 0;
        nbytesSend = 0;
        transmissionStartedSend = false;
        return;
      }
      nbytesSend++;
    }
  }
  else
  {
    //Canal en reposo
    digitalWrite(TX_PIN_SEND, 1);
  }
}