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
RegulusFocuser::RegulusFocuser() : modbus_f("/dev/indi-regulusfocuser", 19200, 'N', 8, 1)
//RegulusFocuser::RegulusFocuser() : modbus_f("/dev/ttyUSB0", 19200, 'N', 8, 1)
{
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED);
	#ifdef HASGEARBOX
		gearboxFactor = GEARBOXMULTIPLIER;
    #else
		gearboxFactor = 1;
    #endif
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
}

/************************************************************************************
 *
************************************************************************************/
bool RegulusFocuser::initProperties()
{
    INDI::Focuser::initProperties();


    RemoteControlSP[REMOTECONTROL_ENABLE].fill("Enabled", "Enabled", ISS_OFF);
    RemoteControlSP[REMOTECONTROL_DISABLE].fill("Disabled", "Disabled", ISS_ON);
    RemoteControlSP.fill(getDeviceName(), "RemoteCtrl", "RemoteCtrl", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    ResetSP[0].fill("Reset", "Reset", ISS_OFF);
    ResetSP.fill(getDeviceName(), "Reset", "Reset", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    FocuserFaultLP[0].fill("FocuserFault", "", IPS_IDLE);
    FocuserFaultLP.fill(getDeviceName(), "FocuserFault", "", MAIN_CONTROL_TAB, IPS_IDLE);

    IUFillNumber(&FocusMaxPosN[0], "FOCUS_MAX_VALUE", "Steps", "%.f", 1, 150000, 1, 50000);
    IUFillNumberVector(&FocusMaxPosNP, FocusMaxPosN, 1, getDeviceName(), "FOCUS_MAX", "Max. Position",
                       MAIN_CONTROL_TAB, IP_RO, 60, IPS_OK);

    FocusSpeedN[0].min   = 1;
    FocusSpeedN[0].max   = 15;
    FocusSpeedN[0].step  = 1;
    FocusSpeedN[0].value = 1;

    FocusAbsPosN[0].max = 5000000;
    FocusAbsPosN[0].min = 0;
    FocusAbsPosN[0].value = 0;

    internalTicks = FocusAbsPosN[0].value;

//	SetFocuserBacklashEnabled(true);
//	SetFocuserBacklash(5000);

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
        defineProperty(RemoteControlSP);
        defineProperty(ResetSP);
        defineProperty(FocuserFaultLP);
    }
    else
    {
        deleteProperty(RemoteControlSP);
        deleteProperty(ResetSP);
        deleteProperty(FocuserFaultLP);
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
                    LOG_ERROR("FOCUS unknown motion direction.");
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetSwitch(&FocusMotionSP, "Unknown motion direction %d", motionIndex);
            }
            FocusMotionSP.s = IPS_OK;
            IDSetSwitch(&FocusMotionSP, nullptr);
            return true;
        }

        if ( (strcmp(RemoteControlSP.getName(), name) == 0) && (modbus_f.registry_buffer[REGFAULT] == 0) )
        {
			RemoteControlSP.update(states, names, n);
            int remoteControlIndex = RemoteControlSP.findOnSwitchIndex();

            switch(remoteControlIndex)
            {
                case REMOTECONTROL_ENABLE:
                    SendCommand(CMDREMOTEENABLE);
                    LOG_WARN("Remote ON.");
                    break;
                case REMOTECONTROL_DISABLE:
                    SendCommand(CMDREMOTEDISABLE);
                    LOG_WARN("Remote OFF.");
                    break;
                default:
                    LOG_WARN("RemoteControl unknown status.");
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetSwitch(&FocusMotionSP, "Unknown value for remote control %d", remoteControlIndex);
            }

            RemoteControlSP.setState(IPS_OK);
            RemoteControlSP.apply();
            return true;
        }

        if (strcmp(ResetSP.getName(), name) == 0)
        {
			ResetSP.update(states, names, n);
            int resetIndex = ResetSP.findOnSwitchIndex();

            switch(resetIndex)
            {
                case 0:
					RemoteControlSP.setState(IPS_ALERT);
					FocuserFaultLP[0].setState(IPS_ALERT);
					ResetSP.setState(IPS_BUSY);
					RemoteControlSP.apply();
					ResetSP.apply();

					FocusAbsPosNP.s = IPS_ALERT;
					FocusMaxPosNP.s = IPS_ALERT;
					FocusSpeedNP.s = IPS_ALERT;
					FocusRelPosNP.s = IPS_ALERT;
					FocusMotionSP.s = IPS_ALERT;
					IDSetNumber(&FocusSpeedNP, nullptr);
					IDSetNumber(&FocusAbsPosNP, nullptr);
					IDSetNumber(&FocusMaxPosNP, nullptr);
					IDSetNumber(&FocusRelPosNP, nullptr);
					IDSetSwitch(&FocusMotionSP, nullptr);
					if( FocuserFaultLP.getState() != IPS_ALERT )
					{
						FocuserFaultLP.setState(IPS_ALERT);
						FocuserFaultLP.apply();
						SendCommand(CMDINIT);
						LOG_WARN("Reset issued.");
					}
					else
					{
						ResetSP.setState(IPS_ALERT);
						ResetSP.apply();
						LOG_ERROR("Can not reset. Device is in fault state.");
						return false;
					}
                    break;
                default:
                    LOG_ERROR("Reset unknown status.");
                    FocusMotionSP.s = IPS_ALERT;
                    IDSetSwitch(&FocusMotionSP, nullptr);
                    IDSetSwitch(&FocusMotionSP, "Unknown value for remote control %d", resetIndex);
            }
//            ResetSP.setState(IPS_OK);
//            ResetSP.apply();
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
    // Let INDI::Focuser handle any other number properties
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}


/************************************************************************************
 *
************************************************************************************/
IPState RegulusFocuser::MoveAbsFocuser(uint32_t targetTicks)
{
    if( (modbus_f.registry_buffer[REGFAULT] == 1) )
        return IPS_ALERT;
        
    FocusAbsPosN[0].value = targetTicks;
    targetTicks *= gearboxFactor;
    modbus_f.registry_buffer[REGREQUESTEDPOSITIONLO]=targetTicks&65535;
    modbus_f.registry_buffer[REGREQUESTEDPOSITIONHI]=targetTicks>>16;
    SendCommand(CMDGOTOPOSITION);
    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
IPState RegulusFocuser::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    if( (modbus_f.registry_buffer[REGFAULT] == 1) )
    {
        return IPS_ALERT;
	}
    if(dir == FOCUS_INWARD)
    {
        SendCommand(CMDRETRACT);
	}
    else
    {
        SendCommand(CMDEXTEND);
	}

	ticks *= gearboxFactor;

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
    SetTimer(TIMERHIT_VALUE);
}

void RegulusFocuser::UpdateValues()
{
    usleep(MODBUSDELAY);
    modbus_f.ReadRegisters(0,NUMBEROFREGISTERS,0);

	FocusMaxPosN[0].value = (modbus_f.registry_buffer[REGMAXSTEPSLO]+modbus_f.registry_buffer[REGMAXSTEPSHI]*65536) / gearboxFactor;
    IDSetNumber(&FocusMaxPosNP, nullptr);

    FocusAbsPosN[0].value = (modbus_f.registry_buffer[REGSTEPPOSITIONLO]+modbus_f.registry_buffer[REGSTEPPOSITIONHI]*65536) / gearboxFactor;
    FocusAbsPosN[0].max = FocusMaxPosN[0].value;
    IDSetNumber(&FocusAbsPosNP, nullptr);

	FocusSpeedN[0].value = modbus_f.registry_buffer[REGFOCUSERSPEED];
    IDSetNumber(&FocusSpeedNP, nullptr);

	FocusRelPosN[0].value = 0;
    IDSetNumber(&FocusRelPosNP, nullptr);
    
    if( (modbus_f.registry_buffer[REGDIRECTION] == 1) && (FocusMotionS[FOCUS_INWARD].s == ISS_ON) )
    {
        FocusMotionS[FOCUS_INWARD].s = ISS_OFF;
        FocusMotionS[FOCUS_OUTWARD].s = ISS_ON;
        IDSetSwitch(&FocusMotionSP, nullptr);
    }

    if( (modbus_f.registry_buffer[REGDIRECTION] == 2) && (FocusMotionS[FOCUS_OUTWARD].s  == ISS_ON) )
    {
        FocusMotionS[FOCUS_INWARD].s = ISS_ON;
        FocusMotionS[FOCUS_OUTWARD].s = ISS_OFF;
        IDSetSwitch(&FocusMotionSP, nullptr);
    }

    if( (modbus_f.registry_buffer[REGREMOTECONTROL] == 1) && (RemoteControlSP[REMOTECONTROL_ENABLE].getState()  == ISS_OFF) )
    {
        RemoteControlSP[REMOTECONTROL_ENABLE].setState(ISS_ON);
        RemoteControlSP[REMOTECONTROL_DISABLE].setState(ISS_OFF);
        RemoteControlSP.apply();
    }

    if( (modbus_f.registry_buffer[REGREMOTECONTROL] == 0) && (RemoteControlSP[REMOTECONTROL_ENABLE].getState()  == ISS_ON) )
    {
        RemoteControlSP[REMOTECONTROL_ENABLE].setState(ISS_OFF);
        RemoteControlSP[REMOTECONTROL_DISABLE].setState(ISS_ON);
        RemoteControlSP.apply();
    }
    
    if( (modbus_f.registry_buffer[REGFAULT] == 1) && (FocuserFaultLP[0].getState() != IPS_ALERT) )
    {
        FocuserFaultLP.setState(IPS_ALERT);
        FocuserFaultLP[0].setState(IPS_ALERT);
        RemoteControlSP.setState(IPS_ALERT);
        ResetSP.setState(IPS_ALERT);
        FocuserFaultLP.apply();
        ResetSP.apply();
        RemoteControlSP.apply();

        FocusAbsPosNP.s = IPS_ALERT;
        FocusMaxPosNP.s = IPS_ALERT;
        FocusSpeedNP.s = IPS_ALERT;
        FocusRelPosNP.s = IPS_ALERT;
        FocusMotionSP.s = IPS_ALERT;
        IDSetNumber(&FocusSpeedNP, nullptr);
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusMaxPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
        IDSetSwitch(&FocusMotionSP, nullptr);
    }
    if( (modbus_f.registry_buffer[REGFAULT] == 0) && (FocuserFaultLP[0].getState() == IPS_ALERT) )
    {
        FocuserFaultLP.setState(IPS_OK);
        FocuserFaultLP[0].setState(IPS_OK);
        RemoteControlSP.setState(IPS_OK);
        ResetSP.setState(IPS_OK);
        FocuserFaultLP.apply();
        ResetSP.apply();
        RemoteControlSP.apply();

        FocusAbsPosNP.s = IPS_OK;
        FocusMaxPosNP.s = IPS_OK;
        FocusSpeedNP.s = IPS_OK;
        FocusRelPosNP.s = IPS_OK;
        FocusMotionSP.s = IPS_OK;
        IDSetNumber(&FocusSpeedNP, nullptr);
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusMaxPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
        IDSetSwitch(&FocusMotionSP, nullptr);
    }
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
		LOG_ERROR("Invalid response from device!");
      printf("Slave returned invalid response code.\n");
      return 0;
    }
  }
  return 0;
}
