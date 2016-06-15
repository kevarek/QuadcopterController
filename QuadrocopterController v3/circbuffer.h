// Header:
// File Name:					circbuffer.h
// File Description:
// Author:						Stanislav Subrt
// Date:							2013
// Todo:
// Notes:

#ifndef	circbuffer_h
#define circbuffer_h


#define CB_OK					-1
#define CB_BUFFER_EMPTY			-2
#define CB_BUFFER_FULL			-3
#define CB_UNDEF_DEST			-4
#define CB_SPECCHAR_NOT_FOUND	-5

typedef struct {
	char *Buffer;					//pointer to buffer array
	unsigned BufferSize;			//size of buffer array

	unsigned HeadIndex;				//points to latest added data
	unsigned TailIndex;				//points to oldest data in buffer

	char SpecialChar;
	unsigned NewSpecialCharReceived;

} CircBufferStructTypedef;


extern void CB_Init(CircBufferStructTypedef *bufStruct, char* bufArray, unsigned bufSize, char specChar);

extern int CB_Add(CircBufferStructTypedef *bufStruct, char ch);
extern int CB_RemoveTail(CircBufferStructTypedef *bufStruct);

extern int CB_AddMultiple(CircBufferStructTypedef *bufStruct, char* s, int cnt);
extern int CB_RemoveExisting(CircBufferStructTypedef *bufStruct, char* destString);
extern int CB_RemoveExistingUpToSpecChar(CircBufferStructTypedef *bufStruct, char* destString);
extern int CB_ReadExisting(CircBufferStructTypedef *bufStruct, char* destString);

extern int CB_GetNumberOfElementsInside(CircBufferStructTypedef *bufStruct);
extern int CB_IsSpecialCharReceived(CircBufferStructTypedef *bufStruct);

#endif	//circbuffer_h
