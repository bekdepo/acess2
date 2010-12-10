/**
 * \file tpl_drv_joystick
 * \brief Joystick Driver Interface Definitions
 * \author John Hodge (thePowersGang)
 * 
 * \section dirs VFS Layout
 * Joystick drivers define a single VFS node, that acts as a fixed size file.
 * Reads from this file return the current state, writes are ignored.
 *
 * \section File Structure
 * The device file must begin with a valid sJoystick_FileHeader structure.
 * The file header is followed by \a NAxies instances of sJoystick_Axis, one
 * for each axis.
 * This is followed by \a NButtons boolean values (represented using \a Uint8),
 * each representing the state of a single button (where 0 is unpressed,
 * 0xFF is fully depressed - intermediate values are valid in the case of
 * variable-pressure buttons)
 */
#ifndef _TPL_JOYSTICK_H
#define _TPL_JOYSTICK_H

#include <tpl_drv_common.h>

/**
 * \enum eTplJoystick_IOCtl
 * \brief Common Joystick IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplJoystick_IOCtl {
	/**
	 * ioctl(..., struct{int Ident;tKeybardCallback *Callback})
	 * \brief Sets the callback
	 * \note Can be called from kernel mode only
	 *
	 * Sets the callback that is called when a event occurs (button or axis
	 * change). This function pointer must be in kernel mode (although,
	 * kernel->user or kernel->ring3driver abstraction functions can be used)
	 */
	JOY_IOCTL_SETCALLBACK = 4,

	/**
	 * ioctl(..., struct{int AxisNum;int Value})
	 * \brief Set maximum value for sJoystick_Axis.CurState
	 */
	JOY_IOCTL_SETAXISLIMIT,
	
	/**
	 * ioctl(..., struct{int AxisNum;int Value})
	 * \brief Set axis flags
	 */
	JOY_IOCTL_SETAXISFLAGS,

	/**
	 * ioctl(..., struct{int ButtonNum;int Value})
	 * \brief Set Button Flags
	 */
	JOY_IOCTL_SETBUTTONFLAGS,
};

/**
 * \brief Callback type for JOY_IOCTL_SETCALLBACK
 * \param Ident	Ident value passed to JOY_IOCTL_SETCALLBACK
 * 
 */
typedef void (*tJoystickCallback)(int Ident, int bIsAxis, int Num, int Delta);

/**
 * \struct sJoystick_FileHeader
 */
struct sJoystick_FileHeader
{
	Uint16	NAxies;
	Uint16	NButtons;
};

struct sJoystick_Axis
{
	Sint16	MinValue;
	Sint16	MaxValue;
	Sint16	CurValue;
	Uint16	CurState;
};

#endif