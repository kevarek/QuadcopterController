/*
*Filename: xxx.h
*Author: Stanislav Subrt
*Year: 2013
*/

#include "settings.h"





void settings_SetDefaults(SettingsStructTypedef* structToBeSetToDefaults, SettingsStructTypedef* structWithDefaults){
/*
	strcpy(s->PortName, "COM1");
	s->BaudRate = 57600;

	s->TransmitterEnabled = 1;
	s->HideFromConsole = 1;
	s->JoyID = 1;
	s->UpdateFreq = 50;
	*/
	memcpy(&structToBeSetToDefaults, &structWithDefaults, sizeof(*structToBeSetToDefaults));
}



void settings_SaveToFile(HANDLE hSetFile, SettingsStructTypedef* s){
	BOOL b;
	DWORD Written;
	if( hSetFile == 0 ) return;
	b = WriteFile(hSetFile, s, sizeof(*s), &Written, NULL);
}



void settings_LoadFromFile(HANDLE hSetFile, SettingsStructTypedef* s){
	DWORD BytesRead = 0;
	if( hSetFile == 0 ) return;
	ReadFile(hSetFile, s, sizeof(*s), &BytesRead, NULL);
}


HANDLE settings_OpenIni( char *filename, SettingsStructTypedef* s, SettingsStructTypedef* defaultsStruct, int IsLoadFromIniRequested){
	DWORD LastError;
	HANDLE hSetFile;
	//open settings file and if it exists
	hSetFile = CreateFile(filename,               // file to open
                       GENERIC_READ | GENERIC_WRITE,          // open for reading and writing
                       0,					 //dont share
                       NULL,                  // default security
                       OPEN_ALWAYS,         //open existing or create new file
                       FILE_ATTRIBUTE_NORMAL, // normal file
                       NULL);                 // no attr. template
 
    if ( hSetFile == INVALID_HANDLE_VALUE )	//if function has failed
    { 
		return 0;
    }
	else{	//createfile has succeded
		LastError = GetLastError();

		if(LastError == ERROR_ALREADY_EXISTS){	//file has been openned - time to read setings
			
			if( IsLoadFromIniRequested ) {
				if( s == 0) return 0;
				settings_LoadFromFile(hSetFile, s);	//now settingsfile contains valid data
			}

		}
		else if( LastError == 0 ){		//file has been created - time to fill default settings and call save
			if( s == 0 || defaultsStruct == 0) return 0;
			settings_SetDefaults(s, defaultsStruct);
			settings_SaveToFile(hSetFile, s);
		}

	}
	return hSetFile;
}



void settings_CloseIni(HANDLE hSetFile){

	CloseHandle(hSetFile);
	//hSetFile = 0;
}