/*
*Filename: xxx.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Windows.h>

#define SETTINGS_PORTNAME_MAXCHARS	10

typedef struct {

	//serial port settings
	char PortName[SETTINGS_PORTNAME_MAXCHARS];
	int BaudRate;

	//Joystick settings
	int TransmitterEnabled;
	int HideFromConsole;
	int JoyID;
	int UpdateFreq;

} SettingsStructTypedef;


extern HANDLE settings_OpenIni( char *filename, SettingsStructTypedef* s, int IsLoadFromIniRequested);
extern void settings_SaveToFile(HANDLE hSetFile, SettingsStructTypedef* s);
extern void settings_CloseIni(HANDLE hSetFile);


#endif  //end of SETTINGS_H




