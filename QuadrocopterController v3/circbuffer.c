// Header:
// File Name:					circbuffer.c
// File Description:
// Author:						Stanislav Subrt
// Date:							2013
// Todo:
// Notes:	BE SUPER ULTRA SURE, THAT BUFFER SIZE IS POWER OF TWO!!! 8, 16, 32, 64, 128, 256B...


#include "circbuffer.h"


//////////////////////////////////////////////////////////////////////
////////// INIT //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


void CB_Init(CircBufferStructTypedef *bufStruct, char* bufArray, unsigned bufSize, char specChar){
	bufStruct->Buffer = bufArray;
	bufStruct->BufferSize = bufSize;

	bufStruct->HeadIndex = 0;
	bufStruct->TailIndex = 0;

	bufStruct->SpecialChar = specChar;
	bufStruct->NewSpecialCharReceived = 0;
}



//////////////////////////////////////////////////////////////////////
///////////////// SINGLE ELEMENT OPERATIONS //////////////////////////
//////////////////////////////////////////////////////////////////////


int CB_Add(CircBufferStructTypedef *bufStruct, char ch){
	int NextHead = (bufStruct->HeadIndex+1) & (bufStruct->BufferSize - 1);

	if( NextHead != (bufStruct->TailIndex) ){	//for safety reasons (interrupt robustness) it is illegal for head and tail to be at same pos
		bufStruct->Buffer[bufStruct->HeadIndex] = ch;
		bufStruct->HeadIndex = NextHead;
		if( (bufStruct->SpecialChar) == ch ){
			bufStruct->NewSpecialCharReceived ++;
		}
		return CB_OK;
	}
	else{
		return CB_BUFFER_FULL;
	}
}


int CB_RemoveTail(CircBufferStructTypedef *bufStruct){
	int ValToReturn;
	if( (bufStruct->TailIndex) != (bufStruct->HeadIndex) ){	//buffer is not empty when tail is not at the same spot as tail
		ValToReturn = bufStruct->Buffer[bufStruct->TailIndex];
		bufStruct->TailIndex = (bufStruct->TailIndex + 1) & (bufStruct->BufferSize - 1);
		if( ValToReturn == (bufStruct->SpecialChar) ){
			bufStruct->NewSpecialCharReceived--;
		}

		return ValToReturn;
	}
	else{	//if tail is at the same spot as head buffer is empty and we cannot manipulate with buffer tail in any way for interrupt safety
		return CB_BUFFER_EMPTY;
	}
}


//////////////////////////////////////////////////////////////////////
/////////////////// MULTIPLE ELEMENTS OPERATIONS /////////////////////
//////////////////////////////////////////////////////////////////////


int CB_AddMultiple(CircBufferStructTypedef *bufStruct, char* s, int cnt){
	int i, status;

	for ( i=0; i<cnt; i++){						//for each char in string
		status = CB_Add(bufStruct, s[i]);		//add char to circ buffer and readout status of operation
		if ( status==CB_BUFFER_FULL ) return CB_BUFFER_FULL;	//if buffer is full return with errornumber
	}
	return CB_OK;								//all characters has been copied -> return success
}


int CB_RemoveExisting(CircBufferStructTypedef *bufStruct, char* destString){
	int c, i = 0;

	if(destString == 0){
		while( CB_RemoveTail(bufStruct) != CB_BUFFER_EMPTY );	//just remove everything and return
		return CB_UNDEF_DEST;
	}
	else{
		while((c=CB_RemoveTail(bufStruct)) != CB_BUFFER_EMPTY){	//if buffer is specified copy it into destString
			*(destString++)=c;
			i++;
		}
	}


	//*(destString++)='\0';
	return i;	//and return
}


int CB_RemoveExistingUpToSpecChar(CircBufferStructTypedef *bufStruct, char* destString){
	int c, i = 0;

	if(destString == 0){	//check if destination string is valid
		return CB_UNDEF_DEST;
	}

	if( !CB_IsSpecialCharReceived(bufStruct) ){
		return CB_SPECCHAR_NOT_FOUND;
	}


	while((c=CB_RemoveTail(bufStruct)) != CB_BUFFER_EMPTY){		//read first element from buffer and save to deststring
		*(destString++)=c;
		i++;
		if( c==(bufStruct->SpecialChar) ) break;				//if character is special end with reading
	}
	//*(destString++)='\0';
	return i;	//return number of characters read
}



//////////////////////////////////////////////////////////////////////
////////// OTHER OPERATIONS //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


int CB_IsSpecialCharReceived(CircBufferStructTypedef *bufStruct){
	return bufStruct->NewSpecialCharReceived;
}

