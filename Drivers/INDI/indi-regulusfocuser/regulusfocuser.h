/*******************************************************************************
    Regulus Focuser
    Copyright (C) 2022 Radmilo Felix

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
   USA

*******************************************************************************/

#pragma once

#include "indifocuser.h"
#include "modbusMaster.h"
#include "FocuserDefinitions.h"

/**
 * @brief The RegulusFocuser class provides a simple Focuser simulator that can simulator the following devices:
 * + Absolute Focuser with encoders.
 * + Relative Focuser.
 * + Simple DC Focuser.
 *
 * The focuser type must be selected before establishing connection to the focuser.
 *
 * The driver defines FWHM property that is used in the @ref CCDSim "CCD Simulator" driver to simulate the fuzziness of star images.
 * It can be used to test AutoFocus routines among other applications.
 */

#define TIMERHIT_VALUE 1000 // milliseconds
#define MODBUSDELAY	20000 // milliseconds

class RegulusFocuser : public INDI::Focuser
{
    public:
        RegulusFocuser();
        virtual ~RegulusFocuser() override = default;
        const char *getDefaultName() override;
        void ISGetProperties(const char *dev) override;
        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        ModbusMaster modbus_f;
	void SendCommand(int myCommand);
	int CheckCommand(int myCommand);

    protected:
        bool initProperties() override;
        bool updateProperties() override;
        bool Connect() override;
        bool Disconnect() override;
        void TimerHit() override;
        void UpdateValues();
//        virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration) override;
        virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual bool SetFocuserSpeed(int speed) override;
        virtual bool SetFocuserBacklash(int32_t steps) override;
        virtual bool SetFocuserBacklashEnabled(bool enabled) override;

    private:
        double internalTicks { 0 };
        double initTicks { 0 };

        // Seeing in arcseconds
        INumberVectorProperty SeeingNP;
        INumber SeeingN[1];

        // FWHM to be used by CCD driver to draw 'fuzzy' stars
        INumberVectorProperty FWHMNP;
        INumber FWHMN[1];

        // Temperature in celcius degrees
        INumberVectorProperty TemperatureNP;
        INumber TemperatureN[1];


/*
        // Current mode of Focus simulator for testing purposes
        enum
        {
            MODE_ALL,
            MODE_ABSOLUTE,
            MODE_RELATIVE,
            MODE_TIMER,
            MODE_COUNT
        };
        ISwitchVectorProperty ModeSP;
        ISwitch ModeS[MODE_COUNT];
//*/

	enum
	{
		REMOTECONTROL_ENABLE,
		REMOTECONTROL_DISABLE,
                REMOTECONTROL_COUNT
	};
        ISwitchVectorProperty RemoteControlSP;
        ISwitch RemoteControlS[REMOTECONTROL_COUNT];

        ISwitchVectorProperty ResetSP;
        ISwitch ResetS[1];

        ILight FocuserFaultL[1];
        ILightVectorProperty FocuserFaultLP;	
};