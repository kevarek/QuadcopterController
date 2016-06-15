/*
*Filename: joystick.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef JOYSTICK_H
#define JOYSTICK_H




typedef struct{
	double X;
	double Y;
	double Z;
	double Throttle;

	int Btn1;
	int Btn2;
	int Btn3;
	int Btn4;
} JoyStatusTypedef;

extern int UpdateJoyStatus(JoyStatusTypedef* joyStatus, unsigned jID);

#endif  //end of JOYSTICK_H
