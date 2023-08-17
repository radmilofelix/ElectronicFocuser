#ifndef FOCUSERSTEPPER___H
#define FOCUSERSTEPPER___H

#include <Arduino.h>

// Travel distance: 17.5mm
// Travel distance per big wheel revolution: 13mm
// Steps per revolution (1/256 microstepping): 102400
// Total steps (1/256 microstepping): 138240
// Travel per step (1/256 microstepping): 126.953125nm

#define EN  PB4     
#define M1  PB6 // A4988 MS1; DRV8825 M0; STSPIN220 MODE1; STSPIN820 M1       
#define M2  PB7 // A4988 MS2; DRV8825 M1; STSPIN220 MODE2; STSPIN820 M2 
#define M3  PB8 // A4988 MS3; DRV8825 M2; STSPIN220 (1); STSPIN820 M3 
#define STDBY  PB5
#define STEP  PB1
#define DIR  PB3
#define RESET PB5

#define DRIVERA4988 1
#define DRIVERDRV8825 2
#define DRIVERSTSPIN220 3
#define DRIVERSTSPIN820 4
#define LIMITSWITCHPIN PB12
//#define LIMITSWITCHNORMALLYCLOSED


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////  Enable one of the focuser models
//#define SCT_MCT
#define PHOTON_254
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SCT_MCT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// Crayford 2 " Focuser for SCT & MCT - parameters
 #define FOCUSERTRAVEL 17.5 // mm
 #define FOCUSERTRAVELPER360 13 // mm
// UnscrewToRetract
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

#ifdef PHOTON_254
////// Crayford 2 " Focuser for Newt. Photon 254 f:4 - parameters
 #define FOCUSERTRAVEL 50 // mm
 #define FOCUSERTRAVELPER360 13 // mm
// UnscrewToRetract
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

////// Motor parameters
// #define MOTORRESOLUTION 0.9 // degrees per step
 #define MOTORRESOLUTION 1.8 // degrees per step

// #define MAXMICROSTEPPING 128
 #define MAXMICROSTEPPING 256

 #define GEARRATIO 1 // Direct drive
 //#define GEARRATIO 100 // Gearbox ratio
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define MAXSTEPS 131000

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FocuserStepper
{
    public:
    FocuserStepper();
    ~FocuserStepper();
	int PulseStep (unsigned long numberOfSteps, int motionDirection, int duration);
	void PulseStepToTarget ();
	void RelativePulseStepToTarget(unsigned long relativeSteps);
	int ToneStep (unsigned long numberOfSteps, int motionDirection, unsigned long frequency);
	void SelectMicrostepMode(int microstep);
	void ValidateMicrostepMode();
	void EnableMotor(bool value);
	void SetMotorDirection(int direction);
	void SetFocuserSpeed(int index);
	void CorrelateSpeed(unsigned long numberOfSteps);
	
	int motorDirection; // -1 = retract; 1 = extend
	int coilConfiguration; // 1 = normal; 2 = reversed
	bool motorEnable;
	unsigned long steps;
	unsigned long freq;
	int halfCycleDuration; // step half cycle duration in microseconds
	int microstepping;
	int ms1;
	int ms2;
	int ms3;
	int tempdir,tempstep;
	int motorDriver;

	int speedIndex; // index for focuser speeds
	int optimalSpeed; // optimal maximal speed to be used
//	int microsteppingIndex; // index for microstepping value 
	long stepPosition; // focuser position in steps
//	long lostSteps; // step difference when focuse is at 0 position
	long stepTarget; // target position for focuser
	long stepRate; // number of step per encoder tick
//	int stepsPerRevolution; 
//	float maxTravel; // um
//	float travelPerRevolution; //mm
	unsigned long maxSteps; // maximal number of steps in use
//	int maxStepping; // ????
	unsigned long maxStepsAbsolute; // maximum steps calculated from FOCUSERTRAVEL, FOCUSERTRAVELPER360, MOTORRESOLUTION, MAXMICROSTEPPING and GEARRATIO values
	
};








#endif
