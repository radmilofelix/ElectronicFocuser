/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "regulusfocuser.h"

#include <cmath>
#include <memory>
#include <cstring>
#include <unistd.h>

// We declare an auto pointer to RegulusFocuser.
static std::unique_ptr<RegulusFocuser> regulusFocuser(new RegulusFocuser());

// Focuser takes 100 microsecond to move for each step, completing 100,000 steps in 10 seconds
#define FOCUS_MOTION_DELAY 100

/************************************************************************************
 *
************************************************************************************/
RegulusFocuser::RegulusFocuser() : modbus_f("/dev/ttyUSB0", 19200, 'N', 8, 1)
{
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED | FOCUSER_HAS_BACKLASH);
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::Connect()
{
    SetTimer(1000);
    if(modbus_f.Connect() < 0)
    {
        printf("Connection to RS485 device failed! Abandoning program.\r\n");
        return false;
    }
    modbus_f.SetSlave(1);
    modbus_f.SetResponseTimeout(10,0);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::Disconnect()
{
    return true;
}

/************************************************************************************
 *
************************************************************************************/
const char *RegulusFocuser::getDefaultName()
{
    return "Regulus Focuser";
}

/************************************************************************************
 *
************************************************************************************/
void RegulusFocuser::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    INDI::Focuser::ISGetProperties(dev);

/*
    defineProperty(&ModeSP);
    loadConfig(true, "Mode");
*/
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::initProperties()
{
    INDI::Focuser::initProperties();


    IUFillSwitch(&RemoteControlS[REMOTECONTROL_ENABLE], "Enabled", "Enabled", ISS_ON);
    IUFillSwitch(&RemoteControlS[REMOTECONTROL_DISABLE], "Disabled", "Disabled", ISS_OFF);
    IUFillSwitchVector(&RemoteControlSP, RemoteControlS, REMOTECONTROL_COUNT, getDeviceName(),
                    "RemoteCtrl", "RemoteCtrl", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&ResetS[0], "Reset", "Reset", ISS_OFF);
    IUFillSwitchVector(&ResetSP, ResetS, 1, getDeviceName(),
                    "Reset", "Reset", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillLight(&FocuserFaultL[0], "FocuserFault", "", IPS_IDLE);
    IUFillLightVector(&FocuserFaultLP, FocuserFaultL, 1, getDeviceName(), "FocuserFault", "", MAIN_CONTROL_TAB, IPS_IDLE);

    IUFillNumber(&SeeingN[0], "SIM_SEEING", "arcseconds", "%4.2f", 0, 60, 0, 3.5);
    IUFillNumberVector(&SeeingNP, SeeingN, 1, getDeviceName(), "SEEING_SETTINGS", "Seeing", MAIN_CONTROL_TAB, IP_RW, 60,
                       IPS_IDLE);

    IUFillNumber(&FWHMN[0], "SIM_FWHM", "arcseconds", "%4.2f", 0, 60, 0, 7.5);
    IUFillNumberVector(&FWHMNP, FWHMN, 1, getDeviceName(), "FWHM", "FWHM", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50., 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    IUFillNumber(&FocusMaxPosN[0], "FOCUS_MAX_VALUE", "Steps", "%.f", 1, 150000, 1, 50000);
    IUFillNumberVector(&FocusMaxPosNP, FocusMaxPosN, 1, getDeviceName(), "FOCUS_MAX", "Max. Position",
                       MAIN_CONTROL_TAB, IP_RO, 60, IPS_OK);

/*
    IUFillSwitch(&ModeS[MODE_ALL], "All", "All", ISS_ON);
    IUFillSwitch(&ModeS[MODE_ABSOLUTE], "Absolute", "Absolute", ISS_OFF);
    IUFillSwitch(&ModeS[MODE_RELATIVE], "Relative", "Relative", ISS_OFF);
    IUFillSwitch(&ModeS[MODE_TIMER], "Timer", "Timer", ISS_OFF);
    IUFillSwitchVector(&ModeSP, ModeS, MODE_COUNT, getDeviceName(), "Mode", "Mode", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 60, IPS_IDLE);
//*/

    initTicks = sqrt(FWHMN[0].value - SeeingN[0].value) / 0.75;

    FocusSpeedN[0].min   = 1;
    FocusSpeedN[0].max   = 15;
    FocusSpeedN[0].step  = 1;
    FocusSpeedN[0].value = 1;

    FocusAbsPosN[0].max = 150000;
    FocusAbsPosN[0].min = 0;
    FocusAbsPosN[0].value = 0;

    internalTicks = FocusAbsPosN[0].value;

    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineProperty(&RemoteControlSP);
        defineProperty(&ResetSP);
        defineProperty(&FocuserFaultLP);
        defineProperty(&SeeingNP);
        defineProperty(&FWHMNP);
        defineProperty(&TemperatureNP);
    }
    else
    {
        deleteProperty(RemoteControlSP.name);
        deleteProperty(ResetSP.name);
        deleteProperty(FocuserFaultLP.name);
        deleteProperty(SeeingNP.name);
        deleteProperty(FWHMNP.name);
        deleteProperty(TemperatureNP.name);
    }

    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {

/*
        // Modes
        if (strcmp(ModeSP.name, name) == 0)
        {
            IUUpdateSwitch(&ModeSP, states, names, n);
            uint32_t cap = 0;
            int index    = IUFindOnSwitchIndex(&ModeSP);

            switch (index)
            {
                case MODE_ALL:
                    cap = FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED;
                    break;

                case MODE_ABSOLUTE:
                    cap = FOCUSER_CAN_ABS_MOVE;
                    break;

                case MODE_RELATIVE:
                    cap = FOCUSER_CAN_REL_MOVE;
                    break;

                case MODE_TIMER:
                    cap = FOCUSER_HAS_VARIABLE_SPEED;
                    break;

                default:
                    ModeSP.s = IPS_ALERT;
                    IDSetSwitch(&ModeSP, "Unknown mode index %d", index);
                    return true;
            }

            FI::SetCapability(cap);
            ModeSP.s = IPS_OK;
            IDSetSwitch(&ModeSP, nullptr);
            return true;
        }
//*/

        if ( (strcmp(FocusMotionSP.name, name) == 0) && (modbus_f.registry_buffer[REGFAULT] == 0) )
        {
            IUUpdateSwitch(&FocusMotionSP, states, names, n);
            int motionIndex = IUFindOnSwitchIndex(&FocusMotionSP);
            switch(motionIndex)
            {
                case FOCUS_INWARD:
                    SendCommand(CMDRETRACT);
//                    LOG_WARN("FOCUS inward.");
                    break;
                case FOCUS_OUTWARD:
                    SendCommand(CMDEXTEND);
//                    LOG_ERROR("FOCUS outward.");
                    break;
                default:
                    LOG_ERROR("FOCUS default.");
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetSwitch(&FocusMotionSP, "Unknown motion direction %d", motionIndex);
            }
            FocusMotionSP.s = IPS_OK;
            IDSetSwitch(&FocusMotionSP, nullptr);
            return true;
        }

        if ( (strcmp(RemoteControlSP.name, name) == 0) && (modbus_f.registry_buffer[REGFAULT] == 0) )
        {
            IUUpdateSwitch(&RemoteControlSP, states, names, n);
            int remoteControlIndex = IUFindOnSwitchIndex(&RemoteControlSP);

            switch(remoteControlIndex)
            {
                case REMOTECONTROL_ENABLE:
                    SendCommand(CMDREMOTEENABLE);
//                    LOG_WARN("Remote ON.");
                    break;
                case REMOTECONTROL_DISABLE:
                    SendCommand(CMDREMOTEDISABLE);
//                    LOG_ERROR("Remote OFF.");
                    break;
                default:
                    LOG_ERROR("RemoteControl default.");
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetSwitch(&FocusMotionSP, "Unknown value for remote control %d", remoteControlIndex);
            }

            RemoteControlSP.s = IPS_OK;
            IDSetSwitch(&RemoteControlSP, nullptr);
            return true;
        }

        if (strcmp(ResetSP.name, name) == 0)
        {
            IUUpdateSwitch(&ResetSP, states, names, n);
            int resetIndex = IUFindOnSwitchIndex(&ResetSP);

            switch(resetIndex)
            {
                case 0:
                    FocuserFaultLP.s  = IPS_ALERT;
                    FocuserFaultL[0].s  = IPS_ALERT;
                    FocusAbsPosNP.s = IPS_ALERT;
                    RemoteControlSP.s = IPS_ALERT;
                    ResetSP.s = IPS_ALERT;
                    FocusMaxPosNP.s = IPS_ALERT;
                    FocusSpeedNP.s = IPS_ALERT;
                    FocusRelPosNP.s = IPS_ALERT;
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetLight(&FocuserFaultLP, nullptr);
                    IDSetNumber(&FocusSpeedNP, nullptr);
                    IDSetNumber(&FocusAbsPosNP, nullptr);
                    IDSetNumber(&FocusMaxPosNP, nullptr);
                    IDSetNumber(&FocusRelPosNP, nullptr);
                    IDSetSwitch(&FocusMotionSP, nullptr);
                    IDSetSwitch(&RemoteControlSP, nullptr);
                    IDSetSwitch(&FocusMotionSP, nullptr);
                    SendCommand(CMDINIT);
//                    LOG_WARN("Reset ON.");
                    break;
                default:
                    LOG_ERROR("Reset default.");
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetSwitch(&FocusMotionSP, "Unknown value for remote control %d", resetIndex);
            }

            ResetSP.s = IPS_OK;
            IDSetSwitch(&ResetSP, nullptr);
            return true;
        }



    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "SEEING_SETTINGS") == 0)
        {
            SeeingNP.s = IPS_OK;
            IUUpdateNumber(&SeeingNP, values, names, n);

            IDSetNumber(&SeeingNP, nullptr);
            return true;
        }

        if (strcmp(name, "FOCUS_TEMPERATURE") == 0)
        {
            TemperatureNP.s = IPS_OK;
            IUUpdateNumber(&TemperatureNP, values, names, n);

            IDSetNumber(&TemperatureNP, nullptr);
            return true;
        }
    }

    // Let INDI::Focuser handle any other number properties
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

/************************************************************************************
 *
************************************************************************************/


/*
IPState RegulusFocuser::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    double mid         = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    int mode           = IUFindOnSwitchIndex(&ModeSP);
    double targetTicks = ((dir == FOCUS_INWARD) ? -1 : 1) * (speed * duration);

    internalTicks += targetTicks;

    if (mode == MODE_ALL)
    {
        if (internalTicks < FocusAbsPosN[0].min || internalTicks > FocusAbsPosN[0].max)
        {
            internalTicks -= targetTicks;
            LOG_ERROR("Cannot move focuser in this direction any further.");
            return IPS_ALERT;
        }
    }

    // simulate delay in motion as the focuser moves to the new position
    usleep(duration * 1000);

    double ticks = initTicks + (internalTicks - mid) / 5000.0;

    FWHMN[0].value = 0.5625 * ticks * ticks + SeeingN[0].value;

    LOGF_DEBUG("TIMER Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
               FWHMN[0].value);

    if (mode == MODE_ALL)
    {
        FocusAbsPosN[0].value = internalTicks;
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    if (FWHMN[0].value < SeeingN[0].value)
        FWHMN[0].value = SeeingN[0].value;

    IDSetNumber(&FWHMNP, nullptr);

    return IPS_OK;
}
//*/


/************************************************************************************
 *
************************************************************************************/
IPState RegulusFocuser::MoveAbsFocuser(uint32_t targetTicks)
{
    if( (modbus_f.registry_buffer[REGFAULT] == 1) )
        return IPS_ALERT;
/*
    double mid = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;

    internalTicks = targetTicks;

    // Limit to +/- 10 from initTicks
    double ticks = initTicks + (targetTicks - mid) / 5000.0;

    // simulate delay in motion as the focuser moves to the new position
    usleep(std::abs((int)(targetTicks - FocusAbsPosN[0].value) * FOCUS_MOTION_DELAY));
*/
    FocusAbsPosN[0].value = targetTicks;

    modbus_f.registry_buffer[REGREQUESTEDPOSITIONLO]=targetTicks&65535;
    modbus_f.registry_buffer[REGREQUESTEDPOSITIONHI]=targetTicks>>16;
    SendCommand(CMDGOTOPOSITION);

/*
    FWHMN[0].value = 0.5625 * ticks * ticks + SeeingN[0].value;
    LOGF_DEBUG("ABS Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
               FWHMN[0].value);

    if (FWHMN[0].value < SeeingN[0].value)
        FWHMN[0].value = SeeingN[0].value;

    IDSetNumber(&FWHMNP, nullptr);
*/
    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
IPState RegulusFocuser::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    if( (modbus_f.registry_buffer[REGFAULT] == 1) )
        return IPS_ALERT;

/*
    uint32_t targetTicks = FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1));
    FocusAbsPosNP.s = IPS_BUSY;
    IDSetNumber(&FocusAbsPosNP, nullptr);
    return MoveAbsFocuser(targetTicks);
//*/
    modbus_f.registry_buffer[REGREQUESTEDPOSITIONLO]=ticks&65535;
    modbus_f.registry_buffer[REGREQUESTEDPOSITIONHI]=ticks>>16;
    SendCommand(CMDGOTORELATIVE);
    return IPS_OK;

}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::SetFocuserSpeed(int speed)
{
    if( (modbus_f.registry_buffer[REGFAULT] == 1) )
        return false;
    SendCommand(speed);
    INDI_UNUSED(speed);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::SetFocuserBacklash(int32_t steps)
{
    INDI_UNUSED(steps);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::SetFocuserBacklashEnabled(bool enabled)
{
    INDI_UNUSED(enabled);
    return true;
}


void RegulusFocuser::TimerHit()
{
	UpdateValues();
/*
    if(!TestConnection())
		CloseConnection();
    if (!isConnected())
        return; //  No need to reset timer if we are not connected anymore
    GetRoofPosition();
    SendMountStatus();
    if(roofMoving !=0)
    {
	if(roofMovementCycles)
	    roofMovementCycles--;
	else
	    roofMoving=0;
    }
//*/
    SetTimer(TIMERHIT_VALUE);
}

void RegulusFocuser::UpdateValues()
{
//  int errors=0;
//  int changes=0;
//  {

    usleep(MODBUSDELAY);
    modbus_f.ReadRegisters(0,NUMBEROFREGISTERS,0);

    FocusAbsPosN[0].value = modbus_f.registry_buffer[REGSTEPPOSITIONLO]+modbus_f.registry_buffer[REGSTEPPOSITIONHI]*65536;
    FocusAbsPosN[0].max = FocusMaxPosN[0].value;
    IDSetNumber(&FocusAbsPosNP, nullptr);

	FocusMaxPosN[0].value = modbus_f.registry_buffer[REGMAXSTEPSLO]+modbus_f.registry_buffer[REGMAXSTEPSHI]*65536;
    IDSetNumber(&FocusMaxPosNP, nullptr);

	FocusSpeedN[0].value = modbus_f.registry_buffer[REGFOCUSERSPEED];
    IDSetNumber(&FocusSpeedNP, nullptr);

	FocusRelPosN[0].value = 0;
    IDSetNumber(&FocusRelPosNP, nullptr);
    
    if( (modbus_f.registry_buffer[REGDIRECTION] == 1) && (FocusMotionS[FOCUS_INWARD].s == ISS_ON) )
    {
        FocusMotionS[FOCUS_INWARD].s  = ISS_OFF;
        FocusMotionS[FOCUS_OUTWARD].s = ISS_ON;
        IDSetSwitch(&FocusMotionSP, nullptr);
    }
    if( (modbus_f.registry_buffer[REGDIRECTION] == 2) && (FocusMotionS[FOCUS_OUTWARD].s  == ISS_ON) )
    {
        FocusMotionS[FOCUS_INWARD].s  = ISS_ON;
        FocusMotionS[FOCUS_OUTWARD].s = ISS_OFF;
        IDSetSwitch(&FocusMotionSP, nullptr);
    }

    if( (modbus_f.registry_buffer[REGREMOTECONTROL] == 1) && (RemoteControlS[REMOTECONTROL_ENABLE].s  == ISS_OFF) )
    {
        RemoteControlS[REMOTECONTROL_ENABLE].s  = ISS_ON;
        RemoteControlS[REMOTECONTROL_DISABLE].s = ISS_OFF;
        IDSetSwitch(&RemoteControlSP, nullptr);
    }
    if( (modbus_f.registry_buffer[REGREMOTECONTROL] == 0) && (RemoteControlS[REMOTECONTROL_ENABLE].s  == ISS_ON) )
    {
        RemoteControlS[REMOTECONTROL_ENABLE].s  = ISS_OFF;
        RemoteControlS[REMOTECONTROL_DISABLE].s = ISS_ON;
        IDSetSwitch(&RemoteControlSP, nullptr);
    }
    
    if( (modbus_f.registry_buffer[REGFAULT] == 1) && (FocuserFaultL[0].s != IPS_ALERT) )
//    if( (modbus_f.registry_buffer[REGFAULT] == 1) )
    {
        FocuserFaultLP.s  = IPS_ALERT;
        FocuserFaultL[0].s  = IPS_ALERT;
        FocusAbsPosNP.s = IPS_ALERT;
        RemoteControlSP.s = IPS_ALERT;
        ResetSP.s = IPS_ALERT;
        FocusMaxPosNP.s = IPS_ALERT;
        FocusSpeedNP.s = IPS_ALERT;
        FocusRelPosNP.s = IPS_ALERT;
        FocusMotionSP.s = IPS_ALERT;
        IDSetLight(&FocuserFaultLP, nullptr);
        IDSetNumber(&FocusSpeedNP, nullptr);
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusMaxPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
        IDSetSwitch(&FocusMotionSP, nullptr);
        IDSetSwitch(&RemoteControlSP, nullptr);
        IDSetSwitch(&FocusMotionSP, nullptr);
    }
    if( (modbus_f.registry_buffer[REGFAULT] == 0) && (FocuserFaultL[0].s == IPS_ALERT) )
//    else
    {
        FocuserFaultLP.s  = IPS_OK;
        FocuserFaultL[0].s  = IPS_OK;
        FocusAbsPosNP.s = IPS_OK;
        RemoteControlSP.s = IPS_OK;
        ResetSP.s = IPS_OK;
        FocusMaxPosNP.s = IPS_OK;
        FocusSpeedNP.s = IPS_OK;
        FocusRelPosNP.s = IPS_OK;
        FocusMotionSP.s = IPS_OK;
        IDSetLight(&FocuserFaultLP, nullptr);
        IDSetNumber(&FocusSpeedNP, nullptr);
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusMaxPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
        IDSetSwitch(&FocusMotionSP, nullptr);
        IDSetSwitch(&RemoteControlSP, nullptr);
        IDSetSwitch(&FocusMotionSP, nullptr);
    }

//    errors =  modbus_f.ReadRegisters(0,NUMBEROFREGISTERS,0);
//    changes++;

//#ifdef DEBUG
//    PrintValues();
//#endif
//  }
//  if( !changes )
//    return;
//  changes=0;
//  if(errors)
//    errorCount += errors+1;
}

void RegulusFocuser::SendCommand(int myCommand)
{
  printf("SendCommand: %d\n", myCommand);
  if( (myCommand < 1) && (myCommand > 300))
  {
    printf("\n\n\nInvalid command: %d\n\n\n", myCommand);
    return;
  }
  {
    usleep(MODBUSDELAY);
    modbus_f.registry_buffer[REGCOMMANDFROMPI]=myCommand;
    usleep(MODBUSDELAY);
    modbus_f.WriteRegisters(0,4,0);
  }
  while(! CheckCommand(myCommand));
}

int RegulusFocuser::CheckCommand(int myCommand)
{
  {
    usleep(MODBUSDELAY);
    modbus_f.ReadRegisters(REGRESPONSETOPI,1,REGRESPONSETOPI);
    if(modbus_f.registry_buffer[REGRESPONSETOPI] == myCommand)
    {
      modbus_f.WriteRegister(REGRESPONSETOPI, 0);
      return 1;
    }
    else
    {
      printf("Slave returned invalid response code.\n");
      return 0;
    }
  }
  return 0;
}
