/*
*Filename: serialport.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <Windows.h>
#include "circbuffer.h"

extern int SerialPortOpen(char *portName, int BaudRate,CircBufferStructTypedef *tXBuffer, CircBufferStructTypedef *rXBuffer);
extern void SerialPortClose(void);

extern int SerialPortWrite(char* strToWrite, int charCnt);

extern int IsSerialPortOpenned(void);



void SerialPortStartWriting(CircBufferStructTypedef *buf);
void SerialPortStopWriting(void);
void SerialPortStartReading(CircBufferStructTypedef *buf);
void SerialPortStopReading(void);



#endif  //end of SERIALPORT_H
