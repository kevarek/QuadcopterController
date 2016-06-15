/*
*Filename: joystick.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "joystick.h"

#include <Windows.h>

#define JOYERROR	-1
#define JOYOK		0
int UpdateJoyStatus(JoyStatusTypedef* joyStatus, unsigned jID){
	int AxisMinValue, AxisMaxValue;
	double xr, yr, zr, throttler;

	JOYCAPS JoyCaps;
	JOYINFOEX JoyInfoEx;

	//read joystick capabilities and get axes min and max values - hopefully ranges are same for all axes
	joyGetDevCaps(jID, &JoyCaps, sizeof(JoyCaps));
	AxisMinValue = JoyCaps.wXmin;
	AxisMaxValue = JoyCaps.wXmax;

	//fill joyinfo struct as required for reading joystick status
	JoyInfoEx.dwSize = sizeof(JoyInfoEx);
	JoyInfoEx.dwFlags = JOY_RETURNALL;

	//read joystick status and return if joystick not found or other errors
	if ( joyGetPosEx(jID, &JoyInfoEx) != JOYERR_NOERROR ) return JOYERROR;
	
	//calculate position of joystick in all axes with range -1..+1
	xr = (double)( ( (double)(JoyInfoEx.dwXpos - AxisMinValue) / (double)(AxisMaxValue - AxisMinValue) ) - 0.5 ) *2.0;	//0..100% -> -50..50% -> -100..100% -> -1..1
	yr = (double)( ( (double)(JoyInfoEx.dwYpos - AxisMinValue) / (double)(AxisMaxValue - AxisMinValue) ) - 0.5 ) *2;
	zr=  (double)( ( (double)(JoyInfoEx.dwRpos - AxisMinValue) / (double)(AxisMaxValue - AxisMinValue) ) - 0.5 ) * 2;
	throttler  = 1.0 - (double)( ( (double)(JoyInfoEx.dwZpos - AxisMinValue) / (double)(AxisMaxValue - AxisMinValue) ) );

	//set status of joystick buttons into structure
	if(JoyInfoEx.dwButtons & JOY_BUTTON1)	joyStatus->Btn1 = 1;
	else 	joyStatus->Btn1 = 0;

	if(JoyInfoEx.dwButtons & JOY_BUTTON2)	joyStatus->Btn2 = 1;
	else 	joyStatus->Btn2 = 0;

	if(JoyInfoEx.dwButtons & JOY_BUTTON3)	joyStatus->Btn3 = 1;
	else 	joyStatus->Btn3 = 0;

	if(JoyInfoEx.dwButtons & JOY_BUTTON4)	joyStatus->Btn4 = 1;
	else 	joyStatus->Btn4 = 0;


	//set joystick position and throttle
	joyStatus->X = yr;		//well... this switched x and y axis... just error during startup :) this is fast fix
	joyStatus->Y = xr;
	joyStatus->Z = zr;
	joyStatus->Throttle = throttler;		
	return JOYOK;

}