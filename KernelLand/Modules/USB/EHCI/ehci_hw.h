/*
 * Acess2 EHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ehci_hw.h
 * - ECHI Hardware Header
 */
#ifndef _EHCI_HW_H_
#define _EHCI_HW_H_


typedef struct sEHCI_CapRegs	tEHCI_CapRegs;
typedef struct sEHCI_OpRegs	tEHCI_OpRegs;
typedef struct sEHCI_iTD	tEHCI_iTD;
typedef struct sEHCI_siTD	tEHCI_siTD;
typedef struct sEHCI_qTD	tEHCI_qTD;
typedef struct sEHCI_QH 	tEHCI_QH;

struct sEHCI_CapRegs
{
	Uint8	CapLength;	// Byte offset of Operational registers
	Uint8	_pad;
	Uint16	HCIVersion;	// BCD Version
	/**
	 * Structural Parameters
	 *
	 *  0: 3 = Number of ports on this controller
	 *  4    = Port Power Control
	 *  5: 6 = Reserved (ZERO)
	 *  7    = Port Routing Rules
	 *  8:11 = Number of ports per companion controller
	 * 12:15 = Number of companion controllers
	 * 16    = Port Indicators
	 * 17:19 = Reserved (ZERO)
	 * 20:23 = Debug Port Number
	 * 24:31 = Reserved (ZERO)
	 */
	Uint32	HCSParams;
	/*
	 * Capability Parameters
	 *
	 *  0    = 64-bit Addressing Capability
	 *  1    = Programmable Frame List Flag
	 *  2    = Asyncronous Schedule Park Capability
	 *  3    = Reserved (ZERO)
	 *  4: 7 = Isochronous Scheduling Threshold
	 *  8:15 = EHCI Extended Capabilitys Pointer (0 = None)
	 * 16:31 = Reserved (ZERO)
	 */
	Uint32	HCCParams;
	/*
	 * Companion Port Route Description
	 */
	Uint64	HCSPPortRoute;
};

struct sEHCI_OpRegs
{
	/**
	 * USB Command Register
	 *
	 *  0    = Run/Stop (Stop, Run)
	 *  1    = Host Controller Reset
	 *  2: 3 = Frame List Size (1024 entries, 512, 256, Reserved)
	 *  4    = Periodic Schedule Enable
	 *  5    = Asynchronous Schedule Enable
	 *  6    = Interrupt on Async Advance Doorbell
	 *  7    = Light Host Controller Reset
	 *  8: 9 = Asynchronous Schedule Park Mode Count
	 * 10    = Reserved (ZERO)
	 * 11    = Asynchronous Schedule Park Mode Enable
	 * 12:15 = Reserved (ZERO)
	 * 16:23 = Interrupt Threshold Control
	 * 31:24 = Reserved (ZERO)
	 */
	Uint32	USBCmd;
	/**
	 * USB Status Register
	 *
	 *  0    = USB Interrupt
	 *  1    = USB Error Interrupt
	 *  2    = Port Change Detect
	 *  3    = Frame List Rollover
	 *  4    = Host System Error
	 *  5    = Interrupt on Async Advance
	 *  6:11 = Reserved (ZERO)
	 * 12    = HCHalted
	 * 13    = Reclamation
	 * 14    = Periodic Schedule Status
	 * 15    = Asynchronous Schedule Status
	 * 16:31 = Reserved ?(Zero)
	 */
	volatile Uint32	USBSts;
	/**
	 * USB Interrupt Enable Register
	 *
	 *  0    = USB Interrupt Enable
 	 *  1    = USB Error Interrupt Enable
	 *  2    = Port Change Interrupt Enable
	 *  3    = Frame List Rollover Enable
	 *  4    = Host System Error Enable
	 *  5    = Interrupt on Async Advance Enable
	 *  6:31 = Reserved (Zero)
	 */
	Uint32	USBIntr;
	/**
	 * Current microframe number (14 bits)
	 * 
	 * Bits 14:3 are used as n index into PeridocListBase
	 */
	volatile Uint32	FrIndex;
	/**
	 * Control Data Structure Segment Register
	 *
	 * Most significant 32-bits of all addresses (only used if "64-bit addressing capability" is set)
	 */
	Uint32	CtrlDSSegment;
	/**
	 * Periodic Frame List Base Address Register
	 */
	Uint32	PeridocListBase;
	/**
	 * Current Asynchronous List Address Register
	 */
	Uint32	AsyncListAddr;
	// Padding
	Uint32	_resvd[(0x40-0x1C)/4];
	/**
	 * Configure Flag Register
	 *
	 * - When 0, all ports are routed to a USB1.1 controller
	 * 
	 *  0    = Configure Flag - Driver sets when controller is configured
	 *  1:31 = Reserved (ZERO)
	 */
	Uint32	ConfigFlag;
	/**
	 * Port Status and Control Register
	 * 
	 *  0    = Current Connect Status
	 *  1    = Connect Status Change
	 *  2    = Port Enable
	 *  3    = Port Enable Change
	 *  4    = Over-current Active
	 *  5    = Over-current change
	 *  6    = Force Port Resume
	 *  7    = Suspend
	 *  8    = Port Reset
	 *  9    = Reserved (ZERO)
	 * 10:11 = Line Status (Use to detect non USB2) [USB2, USB2, USB1.1, USB2]
	 * 12    = Port Power
	 * 13    = Port Owner (Set to 1 to give to companion controller)
	 * 14:15 = Port Indicator Control (Off, Amber, Green, Undef)
	 * 16:19 = Port Test Control
	 * 20    = Wake on Connect Enable
	 * 21    = Wake on Disconnect Enable
	 * 22    = Wake on Over-current Enable
	 * 23:31 = Reserved (ZERO)
	 */
	volatile Uint32	PortSC[15];
};

#define USBCMD_Run	0x0001
#define USBCMD_HCReset	0x0002
#define USBCMD_PeriodicEnable	0x0010
#define USBCMD_AsyncEnable	0x0020
#define USBCMD_IAAD	0x0040

#define USBINTR_IOC	0x0001
#define USBINTR_Error	0x0002
#define USBINTR_PortChange	0x0004
#define USBINTR_FrameRollover	0x0008
#define USBINTR_HostSystemError	0x0010
#define USBINTR_IntrAsyncAdvance	0x0020

#define PORTSC_CurrentConnectStatus	0x0001
#define PORTSC_ConnectStatusChange	0x0002
#define PORTSC_PortEnabled	0x0004
#define PORTSC_PortEnableChange	0x0008
#define PORTSC_OvercurrentActive	0x0010
#define PORTSC_OvercurrentChange	0x0020
#define PORTSC_ForcePortResume	0x0040
#define PORTSC_Suspend  	0x0080
#define PORTSC_PortReset	0x0100
#define PORTSC_Reserved1	0x0200
#define PORTSC_LineStatus_MASK	0x0C00
#define PORTSC_LineStatus_SE0   	0x0000
#define PORTSC_LineStatus_Jstate	0x0400
#define PORTSC_LineStatus_Kstate	0x0800
#define PORTSC_LineStatus_Undef 	0x0C00
#define PORTSC_PortPower	0x1000
#define PORTSC_PortOwner	0x2000
#define PORTSC_PortIndicator_MASK	0xC000
#define PORTSC_PortIndicator_Off	0x0000
#define PORTSC_PortIndicator_Amber	0x4000
#define PORTSC_PortIndicator_Green	0x800

// Isochronous (High-Speed) Transfer Descriptor
struct sEHCI_iTD
{
	Uint32	Link;
	struct {
		Uint16	Offset;
		Uint16	LengthSts;
	} Transactions[8];
	// -- 0 --
	// 0:6  - Device
	// 7    - Reserved
	// 8:11 - Endpoint
	// -- 1 --
	// 0:10 - Max packet size
	// 11   - IN/OUT
	Uint32	BufferPointers[8];	// Page aligned, low 12 bits are overloaded
};

// Split Transaction Isochronous Transfer Descriptor
struct sEHCI_siTD
{
	Uint32	Link;
	Uint32	Dest;
	Uint32	uFrame;
	Uint32	StatusLength;
	Uint32	Page0;
	Uint32	Page1;
	Uint32	BackLink;
};

// Queue Element Transfer Descriptor
struct sEHCI_qTD
{
	Uint32	Link;
	Uint32	Link2;	// Used when there's a short packet
	Uint32	Token;
	Uint32	Pages[5];	//First has offset in low 12 bits
	struct {
		void	*Ptr;
		size_t	Length;
	} Impl;
} __attribute__((aligned(32)));
// sizeof = 64

#define QTD_TOKEN_DATATGL	(1<<31)
#define QTD_TOKEN_IOC   	(1<<15)
#define QTD_TOKEN_STS_ACTIVE	(1<< 7)
#define QTD_TOKEN_STS_HALT	(1<< 6)

// Queue Head
struct sEHCI_QH
{
	Uint32	HLink;	// Horizontal link
	Uint32	Endpoint;
	Uint32	EndpointExt;
	Uint32	CurrentTD;
	struct {
		Uint32	Link;
		Uint32	Link2;
		Uint32	Token;
		Uint32	Pages[5];
	}	Overlay;
	struct {
		tEHCI_QH	*Next;
		tUSBHostCb	Callback;
		void	*CallbackData;
		tEHCI_Endpoint	*Endpt;
		
		tEHCI_qTD	*FirstTD;
		tEHCI_qTD	*LastTD;
	} Impl;
} __attribute__((aligned(32)));
// sizeof = 48 (round up to 64)

#define QH_ENDPT_H	(1<<15)

#endif

