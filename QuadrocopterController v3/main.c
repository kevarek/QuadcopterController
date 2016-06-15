/*
*Filename: main.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include <Windows.h>
#include <WindowsX.h>
#include <stdio.h>
#include <stdlib.h>

#include "resource.h"


#include "settings.h"
#include "serialport.h"
#include "joystick.h"


char* port_HandleOpenButton(void);
void jostick_RequestRedraw(void);
void joystick_RedrawIndicators(JoyStatusTypedef *joyStatus, HDC hDC);
void joystick_FormatPacketContent(JoyStatusTypedef *joyStatus, char *destStr);
void console_AppendText(char *str, int charCnt);
void console_RemoveLastN(int charsToRemove);

//handle to main window
HWND hMainWnd;


//settings part
SettingsStructTypedef SettingsStruct;	//structure to hold app settings
HANDLE hSettingsFile;					//handle to settings structure file

//joystick part
JoyStatusTypedef JoystickStatusStruct;

//serial port part
#define RXBUFFERSIZE	0x2000	//must be power of two
CircBufferStructTypedef RXBufferStruct;
char	RXBuffer[RXBUFFERSIZE];

#define TXBUFFERSIZE	0x2000	//must be power of two
CircBufferStructTypedef TXBufferStruct;
char	TXBuffer[TXBUFFERSIZE];

#define REDRAWTIMER	1
#define PORTTIMER	2
#define JOYTIMER	3
#define BUFFERSIZE	10000
BOOL CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
	int TimerFreq, TimerPeriod, i;
	static int TimerID;
	static char tmp[BUFFERSIZE];

	HWND hFocusedWindow;
	HDC hDC;
	PAINTSTRUCT PaintStruct;

	switch(message) {

		case WM_INITDIALOG:
			hSettingsFile = settings_OpenIni("settings.ini", &SettingsStruct, 1);


			SetDlgItemText(hwnd, IDC_EDIT5, SettingsStruct.PortName);
			SetDlgItemInt(hwnd, IDC_EDIT4, SettingsStruct.BaudRate, TRUE);

			SetDlgItemInt(hwnd, IDC_EDIT2, SettingsStruct.JoyID, TRUE);
			SetDlgItemInt(hwnd, IDC_EDIT1, SettingsStruct.UpdateFreq, TRUE);


			CheckDlgButton(hwnd, IDC_CHECK2, SettingsStruct.TransmitterEnabled);
			CheckDlgButton(hwnd, IDC_CHECK3, SettingsStruct.HideFromConsole);
			settings_CloseIni(hSettingsFile);

			CB_Init(&RXBufferStruct, RXBuffer, RXBUFFERSIZE, '\n');
			CB_Init(&TXBufferStruct, TXBuffer, TXBUFFERSIZE, '\n');

			SetTimer(hwnd, REDRAWTIMER, 20, 0);
			SetTimer(hwnd, PORTTIMER, 100, 0);

			//send some fake user interactions with GUI to setup stuff
			SendMessage(hwnd, WM_COMMAND, (WPARAM)(EN_KILLFOCUS<<16) |  IDC_EDIT1, (LPARAM)NULL);	 //set transmitting freq for joystick



			ShowWindow(hwnd, TRUE);

		return TRUE;

		case WM_PAINT:
			hDC = BeginPaint(hwnd, &PaintStruct);
			joystick_RedrawIndicators(&JoystickStatusStruct, hDC);
			EndPaint(hwnd, &PaintStruct);
		return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wparam)) {

				case IDC_BUTTON3:
					i = GetDlgItemText(hMainWnd, IDC_EDIT7, tmp, BUFFERSIZE);
					if( i>0){
						SerialPortWrite(tmp, -1);
						SerialPortWrite("\r\n", -1);
						console_AppendText(tmp, -1);
						console_AppendText("\r\n", -1);
					}
				return TRUE;

				case IDC_BUTTON4:	//open/close port button
					SendMessage(GetDlgItem(hMainWnd, IDC_BUTTON4), WM_SETTEXT, 0, (LPARAM) port_HandleOpenButton());
				return TRUE;

				case IDC_EDIT1:	//update frequency for joystick
					switch(HIWORD(wparam)){
						case EN_KILLFOCUS:	//:EN_CHANGE	//focus lost - time to update timer	//EN_CHANGE
							TimerFreq = GetDlgItemInt(hwnd, IDC_EDIT1, NULL, TRUE);
							if( TimerFreq>50 ) TimerFreq = 50;
							else if( TimerFreq<1 ) TimerFreq = 1;

							TimerPeriod = (int)( (double)1/(double)TimerFreq*1000 );
							SetTimer(hwnd, JOYTIMER, TimerPeriod, 0);
							SetDlgItemInt(hwnd, IDC_EDIT1, (unsigned)TimerFreq, TRUE);
						return TRUE;
					}
				return FALSE;
			}
		return FALSE;


		case WM_TIMER:
			switch (wparam){
				case JOYTIMER:
					UpdateJoyStatus(&JoystickStatusStruct, GetDlgItemInt(hwnd, IDC_EDIT2, NULL, TRUE));
					joystick_FormatPacketContent(&JoystickStatusStruct, tmp);



					if( IsSerialPortOpenned() ){		//if serial port is openned
						if( IsDlgButtonChecked(hwnd, IDC_CHECK2) ){		//if joystick status transmitting is enabled
							SerialPortWrite(tmp, -1);
						}
					}
					if( IsDlgButtonChecked(hwnd, IDC_CHECK2) ){		//if joystick status transmitting is enabled
						if( !IsDlgButtonChecked(hwnd, IDC_CHECK3) ){		//if status should be printed into console

							console_AppendText(tmp, -1);

						}
					}
				//	strcpy(PrevPacket, tmp);
				return TRUE;

				case REDRAWTIMER:
					//request redrawing joystick area
					jostick_RequestRedraw();
					
				return TRUE;

				case PORTTIMER:
					//read incoming data from serial port
					i = CB_RemoveExisting(&RXBufferStruct, tmp);
					if( i > 0){
						console_AppendText(tmp, i);	 //and append into console if at least one char has been read
					}

					if( GetAsyncKeyState(VK_RETURN) ){	//if enter has been pressed since last time
						hFocusedWindow = GetFocus();	//check current focus window

						if( hFocusedWindow == GetDlgItem(hMainWnd, IDC_EDIT1) ){	//if focus is on refresh joystick edit
							SendMessage(hwnd, WM_COMMAND, (WPARAM)(EN_KILLFOCUS<<16) |  IDC_EDIT1, (LPARAM)NULL);	//setup new refresh timer
						}
						else if( hFocusedWindow == GetDlgItem(hMainWnd, IDC_EDIT7) ){	//if focus is at send button
							i = GetDlgItemText(hMainWnd, IDC_EDIT7, tmp, BUFFERSIZE);
							//SetDlgItemText(hMainWnd, IDC_EDIT7, "");		//clear edit field
							if( i>0){
								SerialPortWrite(tmp, -1);
								SerialPortWrite("\r\n", -1);
								console_AppendText(tmp, -1);
								console_AppendText("\r\n", -1);
							}
						}
						else if( hFocusedWindow == GetDlgItem(hMainWnd, IDC_EDIT5) || hFocusedWindow == GetDlgItem(hMainWnd, IDC_EDIT4) ){
							SendMessage(hwnd, WM_COMMAND, (WPARAM)IDC_BUTTON4, (LPARAM)NULL);
						}
						else if( hFocusedWindow == GetDlgItem(hMainWnd, IDC_EDIT2) ){
							SetDlgItemInt(hMainWnd, IDC_EDIT2, GetDlgItemInt(hMainWnd, IDC_EDIT2, NULL, TRUE), TRUE);	//just return carret to first position so user thinks something happend and for consistency
						}
					}

					
				return TRUE;
			}
		return FALSE;

		case WM_CLOSE:

			hSettingsFile = settings_OpenIni("settings.ini", &SettingsStruct, 0);

			GetDlgItemText(hwnd, IDC_EDIT5, SettingsStruct.PortName, SETTINGS_PORTNAME_MAXCHARS);
			SettingsStruct.BaudRate = GetDlgItemInt(hwnd, IDC_EDIT4, NULL, TRUE);
			SettingsStruct.JoyID = GetDlgItemInt(hwnd, IDC_EDIT2, NULL, TRUE);
			SettingsStruct.UpdateFreq = GetDlgItemInt(hwnd, IDC_EDIT1, NULL, TRUE);
			SettingsStruct.TransmitterEnabled = IsDlgButtonChecked(hwnd, IDC_CHECK2);
			SettingsStruct.HideFromConsole = IsDlgButtonChecked(hwnd, IDC_CHECK3);

			settings_SaveToFile(hSettingsFile, &SettingsStruct);
			settings_CloseIni(hSettingsFile);

			DestroyWindow(hMainWnd);
			PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow){
	MSG msg;
	hMainWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, MainWndProc);

	while(GetMessage(&msg, NULL, 0, 0)) {
		if(!IsDialogMessage(hMainWnd, &msg)  ){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}







//////////////////////////////////////////////////////////////////////
////////// UTILITIES /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//returns button text
char* port_HandleOpenButton(void){
	char tmp[100];
	int PrevState;

	PrevState = IsSerialPortOpenned();

	if( PrevState ){
		SerialPortClose();
	}
	else{
		GetDlgItemText(hMainWnd, IDC_EDIT5, tmp, 100);
		SerialPortOpen(tmp, GetDlgItemInt(hMainWnd, IDC_EDIT4, NULL, TRUE), &TXBufferStruct, &RXBufferStruct);
	}


	if( IsSerialPortOpenned() ){

		if( PrevState ){
			console_AppendText("Serial port already openned.\r\n", -1);
		}
		else{
			console_AppendText("Serial port openned.\r\n", -1);
		}
		EnableWindow( GetDlgItem( hMainWnd, IDC_BUTTON3 ), TRUE );	//open/close button
		EnableWindow( GetDlgItem( hMainWnd, IDC_EDIT6 ), TRUE );	//console
		EnableWindow( GetDlgItem( hMainWnd, IDC_EDIT7 ), TRUE );	//cmd line
		EnableWindow( GetDlgItem( hMainWnd, IDC_CHECK2 ), TRUE );	//transmitting
		EnableWindow( GetDlgItem( hMainWnd, IDC_CHECK3 ), TRUE );	//show in console
		return "Close port";
	}
	else{
		if( PrevState ){
			EnableWindow( GetDlgItem( hMainWnd, IDC_BUTTON3 ), FALSE );	//open/close button
			EnableWindow( GetDlgItem( hMainWnd, IDC_EDIT6 ), FALSE );	//console
			EnableWindow( GetDlgItem( hMainWnd, IDC_EDIT7 ), FALSE );	//cmd line
			EnableWindow( GetDlgItem( hMainWnd, IDC_CHECK2 ), FALSE );	//transmitting
			EnableWindow( GetDlgItem( hMainWnd, IDC_CHECK3 ), FALSE );	//show in console
			console_AppendText("Serial port closed.\r\n", -1);
		}
		else{
			console_AppendText("Serial port could not be openned!\r\n", -1);
		}
		EnableWindow( GetDlgItem( hMainWnd, IDC_BUTTON3 ), FALSE );
		return "Open port";
	}
}





void jostick_RequestRedraw(void){
	RECT Rect;
	GetWindowRect( GetDlgItem(hMainWnd, IDC_EDIT3), &Rect);
	MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&Rect, 2);
	InvalidateRect(hMainWnd, &Rect, TRUE);	//request redraw of joystick status area

	GetWindowRect( GetDlgItem(hMainWnd, IDC_EDIT8), &Rect);
	MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&Rect, 2);
	InvalidateRect(hMainWnd, &Rect, TRUE);	//request redraw of joystick status area

	GetWindowRect( GetDlgItem(hMainWnd, IDC_EDIT9), &Rect);
	MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&Rect, 2);
	InvalidateRect(hMainWnd, &Rect, TRUE);	//request redraw of joystick status area
}


#define JOYSTICK_INDICATOR_RADIUS	5
void joystick_RedrawIndicators(JoyStatusTypedef *joyStatus, HDC hDC){
	RECT Rect;
	POINT Coords;
	int Width, Height;
	int x, y, IntVal;




	//first update joystick status - maybe better manual
	//UpdateJoyStatus(&JoystickStatusStruct, GetDlgItemInt(hMainWnd, IDC_EDIT2, NULL, TRUE));

	//now redraw pitch roll indicator and its frame
	GetWindowRect ( GetDlgItem(hMainWnd, IDC_EDIT3), &Rect);
	MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&Rect, 2);
	DrawEdge(hDC, &Rect, EDGE_SUNKEN, BF_RECT);

	GetWindowRect( GetDlgItem(hMainWnd, IDC_EDIT3), &Rect);
	Width = Rect.right - Rect.left;
	Height = Rect.bottom - Rect.top;
	Coords.x = Rect.left;
	Coords.y = Rect.top;
	ScreenToClient(hMainWnd, &Coords);

	x = ( joyStatus->Y * ( Width - 2*JOYSTICK_INDICATOR_RADIUS) / 2 ) + Coords.x + Width / 2;
	y = ( joyStatus->X * ( Height - 2*JOYSTICK_INDICATOR_RADIUS ) / 2 ) + Coords.y + Height / 2;



	if( joyStatus->Btn1 == 1){
		SelectObject(hDC, GetStockObject(DC_PEN));
		SelectObject(hDC, GetStockObject(DC_BRUSH));
		SetDCBrushColor(hDC, RGB(0,255,0));
		SetDCPenColor(hDC, RGB(255,255,255));

	}
	else{
		SelectObject(hDC, GetStockObject(DC_PEN));
		SelectObject(hDC, GetStockObject(DC_BRUSH));
		SetDCBrushColor(hDC, RGB(255,0,0));
		SetDCPenColor(hDC, RGB(255,255,255));
	}


	Ellipse(hDC, x - JOYSTICK_INDICATOR_RADIUS, y - JOYSTICK_INDICATOR_RADIUS, x + JOYSTICK_INDICATOR_RADIUS, y + JOYSTICK_INDICATOR_RADIUS); 


	//now redraw Throttle indicator and its frame
	GetWindowRect ( GetDlgItem(hMainWnd, IDC_EDIT9), &Rect);
	MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&Rect, 2);
	DrawEdge(hDC, &Rect, EDGE_SUNKEN, BF_RECT);

	GetWindowRect( GetDlgItem(hMainWnd, IDC_EDIT9), &Rect);
	Width = Rect.right - Rect.left;
	Height = Rect.bottom - Rect.top;
	Coords.x = Rect.left;
	Coords.y = Rect.top;
	ScreenToClient(hMainWnd, &Coords);

	y = Coords.y + Height - ( joyStatus->Throttle * ( Height  ) );


	IntVal = (joyStatus->Throttle)*255;
	SelectObject(hDC, GetStockObject(DC_PEN));
	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(hDC, RGB(IntVal, 255 - IntVal, 0));
	SetDCPenColor(hDC, RGB(255,255,255));

	Rectangle(hDC, Coords.x + 1, y, Coords.x + Width - 1, Coords.y + Height);

	//now redraw yaw indicator and its frame
	GetWindowRect ( GetDlgItem(hMainWnd, IDC_EDIT8), &Rect);
	MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&Rect, 2);
	DrawEdge(hDC, &Rect, EDGE_SUNKEN, BF_RECT);

	GetWindowRect( GetDlgItem(hMainWnd, IDC_EDIT8), &Rect);
	Width = Rect.right - Rect.left;
	Height = Rect.bottom - Rect.top;
	Coords.x = Rect.left;
	Coords.y = Rect.top;
	ScreenToClient(hMainWnd, &Coords);

	x = ( joyStatus->Z * ( Width ) / 2 ) + Coords.x + Width / 2;

	IntVal = joyStatus->Z * 255;
	if(IntVal < 0) IntVal*=-1;

	SelectObject(hDC, GetStockObject(DC_PEN));
	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(hDC, RGB(IntVal, 255 - IntVal, 0));
	SetDCPenColor(hDC, RGB(255,255,255));

	Rectangle(hDC, Coords.x + Width / 2 - 1, Coords.y + 1, x + 1, Coords.y + Height -1);

	CheckDlgButton(hMainWnd, IDC_CHECK1, joyStatus->Btn1);
	CheckDlgButton(hMainWnd, IDC_CHECK4, joyStatus->Btn2);
	CheckDlgButton(hMainWnd, IDC_CHECK5, joyStatus->Btn3);
	CheckDlgButton(hMainWnd, IDC_CHECK6, joyStatus->Btn4);

}

void joystick_FormatPacketContent(JoyStatusTypedef *joyStatus, char *destStr){
	//format packet content M+XXXX+YYYY+ZZZZ+TTTT1234
	sprintf(destStr, "M%+06d%+06d%+06d%+06d%01d%01d%01d%01d\r\n",(int)(joyStatus->X * 10000 ), (int)(joyStatus->Y * 10000 ), (int)(joyStatus->Z * 10000 ), (int)(joyStatus->Throttle * 10000 ), joyStatus->Btn1, joyStatus->Btn2, joyStatus->Btn3, joyStatus->Btn4);
}


void console_AppendText(char *str, int charCnt){
	int TextLength;
	HWND ConsoleHwnd;
	DWORD StartPos, EndPos;



	static char PrevPacket[BUFFERSIZE] = "";
	static int j = 0;
	char RepeatStr[100];
	static int PrevCharCnt = 0;


	if( charCnt >0 ){
		str[charCnt] = '\0';	//ooooh very very ugly, but my buffer is much larger than expected amount of data to read
	}

	//dont show identical packets in row in console
	if( strcmp(str, PrevPacket) == 0 ){		//same
								
		j++;	//increment repetition counter

		//show number in console
		console_RemoveLastN( PrevCharCnt );	//removes cr and lf and count as well

		PrevCharCnt = sprintf(str, "Duplicated packet - transmitted but not shown(%d)\r\n", j);	//format string with repete number

	}
	else{	//different
		j = 0;
		PrevCharCnt = 0;
		strcpy(PrevPacket, str);
	}




	ConsoleHwnd = GetDlgItem( hMainWnd, IDC_EDIT6 );
	TextLength = GetWindowTextLength( ConsoleHwnd );

	SendMessage( ConsoleHwnd, EM_GETSEL, (WPARAM)&StartPos, (LPARAM)&EndPos );	//save current selection
	SendMessage( ConsoleHwnd, EM_SETSEL, TextLength, TextLength );				//move the caret to the end of the text
	SendMessage( ConsoleHwnd, EM_REPLACESEL, FALSE, (LPARAM)str );				//insert the text at the new caret position
	SendMessage( ConsoleHwnd, EM_SETSEL, StartPos, EndPos );					// restore the previous selection
}



void console_RemoveLastN(int charsToRemove){
	int TextLength;
	HWND ConsoleHwnd;
	DWORD StartPos, EndPos;

	if( charsToRemove <=0 ){
		return;
	}

	ConsoleHwnd = GetDlgItem( hMainWnd, IDC_EDIT6 );
	TextLength = GetWindowTextLength( ConsoleHwnd );

	SendMessage( ConsoleHwnd, EM_GETSEL, (WPARAM)&StartPos, (LPARAM)&EndPos );			//save current selection
	SendMessage( ConsoleHwnd, EM_SETSEL, TextLength - charsToRemove, TextLength );		//select last n chars
	SendMessage( ConsoleHwnd, EM_REPLACESEL, FALSE, (LPARAM)"" );						//erase selection
	SendMessage( ConsoleHwnd, EM_SETSEL, StartPos, EndPos );							// restore the previous selection
}