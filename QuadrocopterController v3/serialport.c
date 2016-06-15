/*
*Filename: serialport.h
*Author: Stanislav Subrt
*Year: 2013
*/

#include "serialport.h"
#include <stdio.h>


static int IsPortOpenned = 0;
static HANDLE hPort;
static HANDLE ReadingThreadHandle = 0;
static HANDLE WritingThreadHandle = 0;
static HANDLE DataReadyToSendEvent = 0;

static CircBufferStructTypedef *TXBufferStructPtr;

#define BUFFERSIZE	10000
#define PORT_OK			-1
#define PORT_ERROR		-2


int SerialPortOpen(char *portName, int BaudRate,CircBufferStructTypedef *tXBuffer, CircBufferStructTypedef *rXBuffer){
	char PortPrefix[100] = "\\\\.\\";
	char tmp[100];

	DCB PortState;
	COMMTIMEOUTS PortTimeOut;  
	

//	DWORD err;

	sprintf(tmp, "%s%s", PortPrefix, portName);

	IsPortOpenned = 0;
	hPort = CreateFile(tmp ,  GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (hPort == INVALID_HANDLE_VALUE){
		//err = GetLastError();
		return PORT_ERROR;
	}
	else{
		//PORT_OK;
	}

	if (GetCommState(hPort, &PortState) == 0)
	{
		return PORT_ERROR;
	   //err = GetLastError();
	}

	//fill config structure with settings
	PortState.BaudRate = BaudRate;
	PortState.StopBits = ONESTOPBIT;
	PortState.Parity = NOPARITY;
	PortState.ByteSize = 8;

	//set configuration
	if (SetCommState(hPort,&PortState) == 0){
		return PORT_ERROR;
		//err = GetLastError();
	}

               
	PortTimeOut.ReadIntervalTimeout = 3;			// Specify time-out between charactor for receiving.
	PortTimeOut.ReadTotalTimeoutMultiplier = 3;		// Specify value that is multiplied by the requested number of bytes to be read. 
	PortTimeOut.ReadTotalTimeoutConstant = 2;		// Specify value is added to the product of the ReadTotalTimeoutMultiplier member
	PortTimeOut.WriteTotalTimeoutMultiplier = 3;	// Specify value that is multiplied by the requested number of bytes to be sent. 
	PortTimeOut.WriteTotalTimeoutConstant = 2;		// Specify value is added to the product of the WriteTotalTimeoutMultiplier member
	SetCommTimeouts(hPort,&PortTimeOut);		// set the time-out parameter into device control.
	IsPortOpenned = 1;
	SerialPortStartReading(rXBuffer);
	SerialPortStartWriting(tXBuffer);
	TXBufferStructPtr = tXBuffer;
	return PORT_OK;
}


void SerialPortClose(void){
	CloseHandle(hPort);
	hPort = 0;

	SerialPortStopReading();
	SerialPortStopWriting();

	IsPortOpenned = 0;
}



int SerialPortWrite(char* strToWrite, int charCnt){
	DWORD length;
	BOOL status;


	if( charCnt < 0){
		CB_AddMultiple(TXBufferStructPtr, strToWrite, strlen(strToWrite));
	}
	else{
		CB_AddMultiple(TXBufferStructPtr, strToWrite, charCnt);
	}
	SetEvent(DataReadyToSendEvent);
	return PORT_OK;
}




DWORD WINAPI SerialPortWritingThreadProc(LPVOID lpParameter){
	char tmp[BUFFERSIZE];
	OVERLAPPED Ovlp;
	DWORD BytesToWrite, BytesWritten, Result;

	Ovlp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Ovlp.Offset = 0;
	Ovlp.OffsetHigh = 0;

	while(1){		//forever read data and store them to the buffer

		WaitForSingleObject(DataReadyToSendEvent, INFINITE);

		BytesToWrite = CB_RemoveExisting( (CircBufferStructTypedef*)lpParameter, tmp);
		if (!WriteFile(hPort, tmp, BytesToWrite, NULL, &Ovlp)) {	   // Issue read operation.
			Result = GetLastError();
			if ( Result != ERROR_IO_PENDING){
			// Error in communications; report it.
			}
			else{
				//now we are waiting for results
			}
		}

		WaitForSingleObject(Ovlp.hEvent, INFINITE);

		GetOverlappedResult(hPort, &Ovlp, &BytesWritten, TRUE);

	}
	CloseHandle(Ovlp.hEvent);
	return 0;
}






void SerialPortStartWriting(CircBufferStructTypedef *buf){

	if( WritingThreadHandle == 0 ){	//if thread is not running
		WritingThreadHandle = CreateThread(NULL, NULL, SerialPortWritingThreadProc, (LPVOID)buf, NULL, NULL);
		DataReadyToSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
}

void SerialPortStopWriting(void){
	if( WritingThreadHandle != 0 ){
		TerminateThread(WritingThreadHandle, 0);
		CloseHandle(WritingThreadHandle);
		CloseHandle(DataReadyToSendEvent);
		WritingThreadHandle = 0;
	}
}



DWORD WINAPI SerialPortReadingThreadProc(LPVOID lpParameter){
	char tmp[BUFFERSIZE];
	OVERLAPPED Ovlp;
	DWORD BytesRead, Result;

	Ovlp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Ovlp.Offset = 0;
	Ovlp.OffsetHigh = 0;

	while(1){		//forever read data and store them to the buffer

		if (!ReadFile(hPort, tmp, BUFFERSIZE, NULL, &Ovlp)) {	   // Issue read operation.
			Result = GetLastError();
			if ( Result != ERROR_IO_PENDING){
			// Error in communications; report it.
			}
			else{
				//now we are waiting for results
			}
		}

		WaitForSingleObject(Ovlp.hEvent, INFINITE);

		GetOverlappedResult(hPort, &Ovlp, &BytesRead, TRUE);

		CB_AddMultiple( (CircBufferStructTypedef*)lpParameter, tmp, BytesRead);
	}

	CloseHandle(Ovlp.hEvent);
	return 0;
}




void SerialPortStartReading(CircBufferStructTypedef *buf){

	if( ReadingThreadHandle == 0 ){	//if thread is not running
		ReadingThreadHandle = CreateThread(NULL, NULL, SerialPortReadingThreadProc, (LPVOID)buf, NULL, NULL);
	}
}

void SerialPortStopReading(void){
	if( ReadingThreadHandle != 0 ){
		TerminateThread(ReadingThreadHandle, 0);
		CloseHandle(ReadingThreadHandle);
		ReadingThreadHandle = 0;
	}
}





int IsSerialPortOpenned(void){
	return IsPortOpenned;
}



//////////////////////////////////////////////////////////////////////
////////// OLD JUNK		//////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


/*
//returns number of characters that have been read
int SerialPortReadSync(char* destBuffer, int destBufferSize){
	BOOL Status;
	DWORD BytesRead;

	Status = ReadFile(hPort, destBuffer, destBufferSize, &BytesRead, NULL);

	if( Status != 0 ){	//data received
		IsPortOpenned = 1;
		return BytesRead;
	}
	return 0;
}
*/
