/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/arch/bits.h>
#include <asm/arch/mem.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/usb34xx.h>
#include <asm/arch/led.h>
#include <environment.h>
#include <command.h>
#include <usbdcore.h>
#include <fastboot.h>
#include <twl4030.h>

#if defined(CONFIG_FASTBOOT)

#include "usb_debug_macros.h"

#define CONFUSED() printf ("How did we get here %s %d ? \n", __FILE__, __LINE__)

/* memory mapped registers */
static volatile	u8  *pwr        = (volatile u8  *) OMAP34XX_USB_POWER;
static volatile u16 *csr0       = (volatile u16 *) OMAP34XX_USB_CSR0;
static volatile u8  *index      = (volatile u8  *) OMAP34XX_USB_INDEX;
static volatile u8  *txfifosz   = (volatile u8  *) OMAP34XX_USB_TXFIFOSZ;
static volatile u8  *rxfifosz   = (volatile u8  *) OMAP34XX_USB_RXFIFOSZ;
static volatile u16 *txfifoadd  = (volatile u16 *) OMAP34XX_USB_TXFIFOADD;
static volatile u16 *rxfifoadd  = (volatile u16 *) OMAP34XX_USB_RXFIFOADD;

#define BULK_ENDPOINT 1
static volatile u16 *peri_rxcsr = (volatile u16 *) OMAP34XX_USB_RXCSR(BULK_ENDPOINT);
static volatile u16 *rxmaxp     = (volatile u16 *) OMAP34XX_USB_RXMAXP(BULK_ENDPOINT);
static volatile u16 *rxcount    = (volatile u16 *) OMAP34XX_USB_RXCOUNT(BULK_ENDPOINT);
static volatile u16 *peri_txcsr = (volatile u16 *) OMAP34XX_USB_TXCSR(BULK_ENDPOINT);
static volatile u16 *txmaxp     = (volatile u16 *) OMAP34XX_USB_TXMAXP(BULK_ENDPOINT);
static volatile u8  *bulk_fifo  = (volatile u8  *) OMAP34XX_USB_FIFO(BULK_ENDPOINT);

/* This is the TI USB vendor id */
#define DEVICE_VENDOR_ID  0x0451
/* This is just made up.. */
#define DEVICE_PRODUCT_ID 0xCAFE
/* This is just made up.. */
#define DEVICE_BCD        0x0311;

/* String 0 is the language id */
#define DEVICE_STRING_PRODUCT_INDEX       1
#define DEVICE_STRING_SERIAL_NUMBER_INDEX 2
#define DEVICE_STRING_CONFIG_INDEX        3
#define DEVICE_STRING_INTERFACE_INDEX     4
#define DEVICE_STRING_MANUFACTURER_INDEX  5
#define DEVICE_STRING_MAX_INDEX           DEVICE_STRING_MANUFACTURER_INDEX
#define DEVICE_STRING_LANGUAGE_ID         0x0409 /* English (United States) */

/* Define this to use 1.1 / fullspeed */
/* #define CONFIG_USB_1_1_DEVICE */

/* In high speed mode packets are 512
   In full speed mode packets are 64 */
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE      (0x0040)

/* Same, just repackaged as 
   2^(m+3), 64 = 2^6, m = 3 */
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS_2_0 (6)
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS_1_1 (3)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS (3)

#define CONFIGURATION_NORMAL      1

#define TX_LAST()						\
	*csr0 |= (MUSB_CSR0_TXPKTRDY | MUSB_CSR0_P_DATAEND);	\
	while  (*csr0 & MUSB_CSR0_RXPKTRDY)			\
		udelay(1);			      

#define NAK_REQ() *csr0 |= MUSB_CSR0_P_SENDSTALL
#define ACK_REQ() *csr0 |= MUSB_CSR0_P_DATAEND

#define ACK_RX()  *peri_rxcsr |= (MUSB_CSR0_P_SVDRXPKTRDY | MUSB_CSR0_P_DATAEND)

static u8 fastboot_fifo[MUSB_EP0_FIFOSIZE];
static u16 fastboot_fifo_used = 0;

static unsigned int set_address = 0;
static u8 faddr = 0xff;

static unsigned int high_speed = 0;

static unsigned int deferred_rx = 0;

static struct usb_device_request req;

/* The packet size is dependend of the speed mode
   In high speed mode packets are 512
   In full speed mode packets are 64
   Set to maximum of 512 */
static u8 fastboot_bulk_fifo[0x0200];
static char *device_strings[DEVICE_STRING_MANUFACTURER_INDEX+1];

static struct cmd_fastboot_interface *fastboot_interface = NULL;

#ifdef DEBUG_FASTBOOT
static void fastboot_db_regs(void) 
{
	printf("fastboot_db_regs\n");
	u8 b;
	u16 s;

	/* */
	b = inb (OMAP34XX_USB_FADDR);
	printf ("\tfaddr   0x%2.2x\n", b);

	b = inb (OMAP34XX_USB_POWER);
	PRINT_PWR(b);

	s = inw (OMAP34XX_USB_CSR0);
	PRINT_CSR0(s);

	b = inb (OMAP34XX_USB_DEVCTL);
	PRINT_DEVCTL(b);

	b = inb (OMAP34XX_USB_CONFIGDATA);
	PRINT_CONFIG(b);

	s = inw (OMAP34XX_USB_FRAME);
	printf ("\tframe   0x%4.4x\n", s);
	b = inb (OMAP34XX_USB_INDEX);
	printf ("\tindex   0x%2.2x\n", b);

	s = *rxmaxp;
	PRINT_RXMAXP(s);

	s = *peri_rxcsr;
	PRINT_RXCSR(s);

	s = *txmaxp;
	PRINT_TXMAXP(s);

	s = *peri_txcsr;
	PRINT_TXCSR(s);
}
#endif

static void fastboot_bulk_endpoint_reset (void)
{
	u8 old_index;
	/* save old index */
	old_index = *index;

	/* set index to endpoint */
	*index = BULK_ENDPOINT;
  
	/* Address starts at the end of EP0 fifo, shifted right 3 (8 bytes) */
	*txfifoadd = MUSB_EP0_FIFOSIZE >> 3;
	*rxfifoadd = (MUSB_EP0_FIFOSIZE + TX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS) >> 3;

	/* Size depends on the mode.  Do not double buffer */
	*txfifosz = TX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS;
	if (high_speed)
		*rxfifosz = RX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS_2_0;
	else
		*rxfifosz = RX_ENDPOINT_MAXIMUM_PACKET_SIZE_BITS_1_1;

	/* restore index */
	*index = old_index;
  
	/* Setup Rx endpoint for Bulk OUT */
	*rxmaxp = fastboot_fifo_size();

	/* Flush anything on fifo */
	while (*peri_rxcsr & MUSB_RXCSR_RXPKTRDY)
	{
		*peri_rxcsr |= MUSB_RXCSR_FLUSHFIFO;
		udelay(1);
	}
	/* No dma, enable bulkout,  */
	*peri_rxcsr &= ~(MUSB_RXCSR_DMAENAB | MUSB_RXCSR_P_ISO);
	/* reset endpoint data */
	*peri_rxcsr |= MUSB_RXCSR_CLRDATATOG;
  
	/* Setup Tx endpoint for Bulk IN */
	/* Set max packet size per usb 1.1 / 2.0 */
	*txmaxp = TX_ENDPOINT_MAXIMUM_PACKET_SIZE;

	/* Flush anything on fifo */
	while (*peri_txcsr & MUSB_TXCSR_FIFONOTEMPTY)
	{
		*peri_txcsr |= MUSB_TXCSR_FLUSHFIFO;
		udelay(1);
	}
  
	/* No dma, enable bulkout, no underflow */
	*peri_txcsr &= ~(MUSB_TXCSR_DMAENAB | MUSB_TXCSR_P_ISO | MUSB_TXCSR_P_UNDERRUN);
	/* reset endpoint data, shared fifo with rx */
	*peri_txcsr |= (MUSB_TXCSR_CLRDATATOG | MUSB_TXCSR_MODE);
}

static void fastboot_reset (void)
{
	OMAP3_LED_ERROR_ON ();

	/* Kill the power */
	*pwr &= ~MUSB_POWER_SOFTCONN;
	udelay(2 * 500000); /* 1 sec */

	OMAP3_LED_ERROR_OFF ();

	/* Reset address */
	faddr = 0xff;

	/* Reset */
#ifdef CONFIG_USB_1_1_DEVICE
	*pwr &= ~MUSB_POWER_HSENAB;
	*pwr |= MUSB_POWER_SOFTCONN;
#else
	*pwr |= (MUSB_POWER_SOFTCONN | MUSB_POWER_HSENAB);
#endif
	/* Bulk endpoint fifo */
	fastboot_bulk_endpoint_reset ();
	

	OMAP3_LED_ERROR_ON ();

	/* fastboot_db_regs(); */
}

static u8 read_fifo_8(void)
{
	u8 val;
  
	val = inb (OMAP34XX_USB_FIFO_0);
	return val;
}

static u8 read_bulk_fifo_8(void)
{
	u8 val;
  
	val = *bulk_fifo;
	return val;
}

static void write_fifo_8(u8 val)
{
	outb (val, OMAP34XX_USB_FIFO_0);
}

static void write_bulk_fifo_8(u8 val)
{
	*bulk_fifo = val;
}

static void read_request(void)
{
	int i;
  
	for (i = 0; i < 8; i++)
		fastboot_fifo[i] = read_fifo_8 ();
	memcpy (&req, &fastboot_fifo[0], 8);
	fastboot_fifo_used = 0;

	*csr0 |= MUSB_CSR0_P_SVDRXPKTRDY;

	while  (*csr0 & MUSB_CSR0_RXPKTRDY)
		udelay(1);

}

static int do_usb_req_set_interface(void)
{
	int ret = 0;

	/* Only support interface 0, alternate 0 */
	if ((0 == req.wIndex) &&
	    (0 == req.wValue))
	{
		fastboot_bulk_endpoint_reset ();
		ACK_REQ();
	}
	else
	{
		NAK_REQ();
	}

	return ret;
}

static int do_usb_req_set_address(void)
{
	int ret = 0;
  
	if (0xff == faddr) 
	{
		faddr = (u8) (req.wValue & 0x7f);
		set_address = 1;

		/* Check if we are in high speed mode */
		if (*pwr & MUSB_POWER_HSMODE)
		  high_speed = 1;
		else
		  high_speed = 0;

		ACK_REQ();
	}
	else
	{
		NAK_REQ();
	}

	return ret;
}


static int do_usb_req_set_configuration(void)
{
	int ret = 0;

	if (0xff == faddr) {
		NAK_REQ(); 
	} else {
		if (0 == req.wValue) {
			/* spec says to go to address state.. */
			faddr = 0xff;
			ACK_REQ();
		} else if (CONFIGURATION_NORMAL == req.wValue) {
			/* This is the one! */

			/* Bulk endpoint fifo */
			fastboot_bulk_endpoint_reset();

			ACK_REQ();
		} else {
			/* Only support 1 configuration so nak anything else */
			NAK_REQ();
		}
	}

	return ret;
}

static int do_usb_req_set_feature(void)
{
	int ret = 0;
  
	NAK_REQ();

	return ret;
}

static int do_usb_req_get_descriptor(void)
{
	int ret = 0;
  
	if (0 == req.wLength)
	{
		ACK_REQ();
	}
	else
	{
		unsigned int byteLoop;

		if (USB_DT_DEVICE == (req.wValue >> 8))
		{
			struct usb_device_descriptor d;
			d.bLength = MIN(req.wLength, sizeof (d));
	      
			d.bDescriptorType    = USB_DT_DEVICE;
#ifdef CONFIG_USB_1_1_DEVICE
			d.bcdUSB             = 0x110;
#else
			d.bcdUSB             = 0x200;
#endif
			d.bDeviceClass       = 0xff;
			d.bDeviceSubClass    = 0xff;
			d.bDeviceProtocol    = 0xff;
			d.bMaxPacketSize0    = 0x40;
			d.idVendor           = DEVICE_VENDOR_ID;
			d.idProduct          = DEVICE_PRODUCT_ID;
			d.bcdDevice          = DEVICE_BCD;
			d.iManufacturer      = DEVICE_STRING_MANUFACTURER_INDEX;
			d.iProduct           = DEVICE_STRING_PRODUCT_INDEX;
			d.iSerialNumber      = DEVICE_STRING_SERIAL_NUMBER_INDEX;
			d.bNumConfigurations = 1;
	  
			memcpy (&fastboot_fifo, &d, d.bLength);
			for (byteLoop = 0; byteLoop < d.bLength; byteLoop++) 
				write_fifo_8 (fastboot_fifo[byteLoop]);

			TX_LAST();
		}
		else if (USB_DT_CONFIG == (req.wValue >> 8))
		{
			struct usb_configuration_descriptor c;
			struct usb_interface_descriptor i;
			struct usb_endpoint_descriptor e1, e2;
			unsigned char bytes_remaining = req.wLength;
			unsigned char bytes_total = 0;

			c.bLength             = MIN(bytes_remaining, sizeof (c));
			c.bDescriptorType     = USB_DT_CONFIG;
			/* Set this to the total we want */
			c.wTotalLength = sizeof (c) + sizeof (i) + sizeof (e1) + sizeof (e2); 
			c.bNumInterfaces      = 1;
			c.bConfigurationValue = CONFIGURATION_NORMAL;
			c.iConfiguration      = DEVICE_STRING_CONFIG_INDEX;
			c.bmAttributes        = 0xc0;
			c.bMaxPower           = 0x32;

			bytes_remaining -= c.bLength;
			memcpy (&fastboot_fifo[0], &c, c.bLength);
			bytes_total += c.bLength;
	  
			i.bLength             = MIN (bytes_remaining, sizeof(i));
			i.bDescriptorType     = USB_DT_INTERFACE;	
			i.bInterfaceNumber    = 0x00;
			i.bAlternateSetting   = 0x00;
			i.bNumEndpoints       = 0x02;
			i.bInterfaceClass     = FASTBOOT_INTERFACE_CLASS;
			i.bInterfaceSubClass  = FASTBOOT_INTERFACE_SUB_CLASS;
			i.bInterfaceProtocol  = FASTBOOT_INTERFACE_PROTOCOL;
			i.iInterface          = DEVICE_STRING_INTERFACE_INDEX;

			bytes_remaining -= i.bLength;
			memcpy (&fastboot_fifo[bytes_total], &i, i.bLength);
			bytes_total += i.bLength;

			e1.bLength            = MIN (bytes_remaining, sizeof (e1));
			e1.bDescriptorType    = USB_DT_ENDPOINT;
			e1.bEndpointAddress   = 0x80 | BULK_ENDPOINT; /* IN */
			e1.bmAttributes       = USB_ENDPOINT_XFER_BULK;
			e1.wMaxPacketSize     = TX_ENDPOINT_MAXIMUM_PACKET_SIZE;
			e1.bInterval          = 0x00;

			bytes_remaining -= e1.bLength;
			memcpy (&fastboot_fifo[bytes_total], &e1, e1.bLength);
			bytes_total += e1.bLength;

			e2.bLength            = MIN (bytes_remaining, sizeof (e2));
			e2.bDescriptorType    = USB_DT_ENDPOINT;
			e2.bEndpointAddress   = BULK_ENDPOINT; /* OUT */
			e2.bmAttributes       = USB_ENDPOINT_XFER_BULK;
			if (high_speed)
				e2.wMaxPacketSize = RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0;
			else
				e2.wMaxPacketSize = RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1;
			e2.bInterval          = 0x00;

			bytes_remaining -= e2.bLength;
			memcpy (&fastboot_fifo[bytes_total], &e2, e2.bLength);
			bytes_total += e2.bLength;

			for (byteLoop = 0; byteLoop < bytes_total; byteLoop++) 
				write_fifo_8 (fastboot_fifo[byteLoop]);

			TX_LAST();
		}
		else if (USB_DT_STRING == (req.wValue >> 8))
		{
			unsigned char bLength;
			unsigned char string_index = req.wValue & 0xff;
	  
			if (string_index > DEVICE_STRING_MAX_INDEX)
			{
				/* Windows XP asks for an invalid string index. 
				   Fail silently instead of doing
				
				   NAK_REQ(); 
				*/
			}
			else if (0 == string_index) 
			{
				/* Language ID */
				bLength = MIN(4, req.wLength);
		  
				fastboot_fifo[0] = bLength;        /* length */
				fastboot_fifo[1] = USB_DT_STRING;  /* descriptor = string */
				fastboot_fifo[2] = DEVICE_STRING_LANGUAGE_ID & 0xff;
				fastboot_fifo[3] = DEVICE_STRING_LANGUAGE_ID >> 8;

				for (byteLoop = 0; byteLoop < bLength; byteLoop++) 
					write_fifo_8 (fastboot_fifo[byteLoop]);

				TX_LAST();
			}
			else
			{
				/* Size of string in chars */
				unsigned char s;
				unsigned char sl = strlen (&device_strings[string_index][0]);
				/* Size of descriptor
				   1    : header
				   2    : type
				   2*sl : string */
				unsigned char bLength = 2 + (2 * sl);
				bLength = MIN(bLength, req.wLength);
		  
				fastboot_fifo[0] = bLength;        /* length */
				fastboot_fifo[1] = USB_DT_STRING;  /* descriptor = string */
	      
				/* Copy device string to fifo, expand to simple unicode */
				for (s = 0; s < sl; s++)
				{
					fastboot_fifo[2+ 2*s + 0] = device_strings[string_index][s];
					fastboot_fifo[2+ 2*s + 1] = 0;
				}

				for (byteLoop = 0; byteLoop < bLength; byteLoop++) 
					write_fifo_8 (fastboot_fifo[byteLoop]);

				TX_LAST();
			}
		} else if (USB_DT_DEVICE_QUALIFIER == (req.wValue >> 8)) {

#ifdef CONFIG_USB_1_1_DEVICE
			/* This is an invalid request for usb 1.1, nak it */
			NAK_REQ();
#else
			struct usb_qualifier_descriptor d;
			d.bLength = MIN(req.wLength, sizeof(d));
			d.bDescriptorType    = USB_DT_DEVICE_QUALIFIER;
			d.bcdUSB             = 0x200;
			d.bDeviceClass       = 0xff;
			d.bDeviceSubClass    = 0xff;
			d.bDeviceProtocol    = 0xff;
			d.bMaxPacketSize0    = 0x40;
			d.bNumConfigurations = 1;
			d.bRESERVED          = 0;

			memcpy(&fastboot_fifo, &d, d.bLength);
			for (byteLoop = 0; byteLoop < d.bLength; byteLoop++)
				write_fifo_8(fastboot_fifo[byteLoop]);

			TX_LAST();
#endif
		}
		else
		{
			NAK_REQ();
		}
	}
  
	return ret;
}

static int do_usb_req_get_status(void)
{
	int ret = 0;

	if (0 == req.wLength)
	{
		ACK_REQ();
	}
	else
	{
		/* See 9.4.5 */
		unsigned int byteLoop;
		unsigned char bLength;

		bLength = MIN (req.wValue, 2);
      
		fastboot_fifo[0] = USB_STATUS_SELFPOWERED;
		fastboot_fifo[1] = 0;

		for (byteLoop = 0; byteLoop < bLength; byteLoop++) 
			write_fifo_8 (fastboot_fifo[byteLoop]);

		TX_LAST();
	}
  
	return ret;
}

static int fastboot_poll_h (void)
{
	int ret = 0;
	u16 count0;

	if (*csr0 & MUSB_CSR0_RXPKTRDY) 
	{
		count0 = inw (OMAP34XX_USB_COUNT0);

		if (count0 != 8) 
		{
			OMAP3_LED_ERROR_ON ();
			CONFUSED();
			ret = 1;
		}
		else
		{
			read_request();

			/* Check data */
			if (USB_REQ_TYPE_STANDARD == (req.bmRequestType & USB_REQ_TYPE_MASK))
			{
				/* standard */
				if (0 == (req.bmRequestType & USB_REQ_DIRECTION_MASK))
				{
					/* host-to-device */
					if (USB_RECIP_DEVICE == (req.bmRequestType & USB_REQ_RECIPIENT_MASK))
					{
						/* device */
						switch (req.bRequest)
						{
						case USB_REQ_SET_ADDRESS:
							ret = do_usb_req_set_address();
							break;

						case USB_REQ_SET_FEATURE:
							ret = do_usb_req_set_feature();
							break;

						case USB_REQ_SET_CONFIGURATION:
							ret = do_usb_req_set_configuration();
							break;
			  
						default:
							NAK_REQ();
							ret = -1;
							break;
						}
					}
					else if (USB_RECIP_INTERFACE == (req.bmRequestType & USB_REQ_RECIPIENT_MASK))
					{
						switch (req.bRequest)
						{
						case USB_REQ_SET_INTERFACE:
							ret = do_usb_req_set_interface();
							break;
			      
						default:
							NAK_REQ();
							ret = -1;
							break;
						}
					}
					else
					{
						NAK_REQ();
						ret = -1;
					}
				}
				else
				{
					/* device-to-host */
					if (USB_RECIP_DEVICE == (req.bmRequestType & USB_REQ_RECIPIENT_MASK))
					{
						switch (req.bRequest)
						{
						case USB_REQ_GET_DESCRIPTOR:
							ret = do_usb_req_get_descriptor();
							break;

						case USB_REQ_GET_STATUS:
							ret = do_usb_req_get_status();
							break;

						default:
							NAK_REQ();
							ret = -1;
							break;
						}
					}
					else
					{
						NAK_REQ();
						ret = -1;
					}
				}
			}
			else
			{
				/* Non-Standard Req */
				NAK_REQ();
				ret = -1;
			}
		}
		if (0 > ret)
		{
			printf ("Unhandled req\n");
			PRINT_REQ (req);
		}
	}
  
	return ret;
}

static int fastboot_resume (void)
{
	/* Here because of stall was sent */
	if (*csr0 & MUSB_CSR0_P_SENTSTALL)
	{
		*csr0 &= ~MUSB_CSR0_P_SENTSTALL;
		return 0;
	}

	/* Host stopped last transaction */
	if (*csr0 & MUSB_CSR0_P_SETUPEND)
	{
		/* This should be enough .. */
		*csr0 |= MUSB_CSR0_P_SVDSETUPEND;

#if 0
		if (0xff != faddr)
			fastboot_reset ();

		/* Let the cmd layer to reset */
		if (fastboot_interface &&
		    fastboot_interface->reset_handler) 
		  {
			  fastboot_interface->reset_handler();
		  }

		/* If we were not resetting, dropping through and handling the
		   poll would be fine.  As it is returning now is the 
		   right thing to do here.  */
		return 0;
#endif

	}

	/* Should we change the address ? */
	if (set_address) 
	{
		outb (faddr, OMAP34XX_USB_FADDR);
		set_address = 0;

		/* If you have gotten here you are mostly ok */
		OMAP3_LED_OK_ON ();
	}
  
	return fastboot_poll_h();
}
  
static int fastboot_rx (void)
{
	if (*peri_rxcsr & MUSB_RXCSR_RXPKTRDY)
	{
		u16 count = *rxcount;
		int fifo_size = fastboot_fifo_size();

		if (0 == *rxcount) {
			/* Clear the RXPKTRDY bit */
			*peri_rxcsr &= ~MUSB_RXCSR_RXPKTRDY;
		} else if (fifo_size < count) {
			/* Clear the RXPKTRDY bit */
			*peri_rxcsr &= ~MUSB_RXCSR_RXPKTRDY;

			/* Send stall */
			*peri_rxcsr |= MUSB_RXCSR_P_SENDSTALL;

			/* Wait till stall is sent.. */
			while (! (*peri_rxcsr & MUSB_RXCSR_P_SENTSTALL))
			  udelay(1);
			
			/* Clear stall */
			*peri_rxcsr &= ~MUSB_RXCSR_P_SENTSTALL;
		} else {
			int i;
			int err = 1;
	  
			for (i = 0; i < count; i++)
				fastboot_bulk_fifo[i] = read_bulk_fifo_8 ();

			/* Clear the RXPKTRDY bit */
			*peri_rxcsr &= ~MUSB_RXCSR_RXPKTRDY;

			/* Pass this up to the interface's handler */
			if (fastboot_interface &&
			    fastboot_interface->rx_handler) {
				if (!fastboot_interface->rx_handler (&fastboot_bulk_fifo[0], count))
					err = 0;
			}
			
			/* Since the buffer is not null terminated, poison the buffer */
			memset(&fastboot_bulk_fifo[0], 0, fifo_size);

			/* If the interface did not handle the command */
			if (err) {
				OMAP3_LED_ERROR_ON ();
				CONFUSED();
			}
		}
	}
  
	return 0;
}

static int fastboot_suspend (void)
{
	/* No suspending going on here! 
	   We are polling for all its worth */

	return 0;
}

int fastboot_poll(void) 
{
	int ret = 0;

	u8 intrusb;
	u16 intrtx;
	u16 intrrx;

	/* Look at the interrupt registers */
	intrusb = inb (OMAP34XX_USB_INTRUSB);

	/* A disconnect happended, this signals that the cable
	   has been disconnected, return immediately */
	if (intrusb & OMAP34XX_USB_INTRUSB_DISCON)
	{
		return 1;
	}

	if (intrusb & OMAP34XX_USB_INTRUSB_RESUME)
	{
		ret = fastboot_resume ();
		if (ret)
			return ret;
	}
	else 
	{
		if (intrusb & OMAP34XX_USB_INTRUSB_SOF)
		{
			ret = fastboot_resume ();
			if (ret)
				return ret;

			/* The fastboot client blocks of read and 
			   intrrx is not reliable. 
			   Really poll */
			if (deferred_rx)
				ret = fastboot_rx ();
			deferred_rx = 0;
			if (ret)
				return ret;
			

		}
		if (intrusb & OMAP34XX_USB_INTRUSB_SUSPEND)
		{
			ret = fastboot_suspend ();
			if (ret)
				return ret;
		}

		intrtx = inw (OMAP34XX_USB_INTRTX);
		if (intrtx)
		{
			/* TX interrupts happen when a packet has been sent
			   We already poll the csr register for this when 
			   something is sent, so do not do it twice 

			*/
		}

		intrrx = inw (OMAP34XX_USB_INTRRX);
		if (intrrx)
		{
			/* Defer this to SOF */
			deferred_rx = 1;
		}
	}

	return ret;
}



void fastboot_shutdown(void)
{
	/* Clear the SOFTCONN bit to disconnect */
	*pwr &= ~MUSB_POWER_SOFTCONN;

	/* Reset some globals */
	faddr = 0xff;
	fastboot_interface = NULL;
	high_speed = 0;
}

int fastboot_is_highspeed(void)
{
	int ret = 0;
	if (*pwr & MUSB_POWER_HSMODE)
		ret = 1;
	return ret;
}

int fastboot_fifo_size(void)
{
	return high_speed ? RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0 : RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1;
}

int fastboot_tx_status(const char *buffer, unsigned int buffer_size)
{
	int ret = 1;
	unsigned int i;
	/* fastboot client only reads back at most 64 */
	unsigned int transfer_size = MIN(64, buffer_size);

	while  (*peri_txcsr & MUSB_TXCSR_TXPKTRDY)
		udelay(1);

	for (i = 0; i < transfer_size; i++)
		write_bulk_fifo_8 (buffer[i]);

	*peri_txcsr |= MUSB_TXCSR_TXPKTRDY;

	while  (*peri_txcsr & MUSB_TXCSR_TXPKTRDY)
		udelay(1);

	/* Send an empty packet to signal that we are done */
	TX_LAST();

	ret = 0;

	return ret;
}

int fastboot_getvar(const char *rx_buffer, char *tx_buffer)
{
	/* Place board specific variables here */
	return 0;
}

int fastboot_preboot(void)
{
#if (defined(CONFIG_TWL4030_KEYPAD) && (CONFIG_TWL4030_KEYPAD))
	int i;
	unsigned char key1, key2;
	int keys;
	udelay(CFG_FASTBOOT_PREBOOT_INITIAL_WAIT);
	for (i = 0; i < CFG_FASTBOOT_PREBOOT_LOOP_MAXIMUM; i++) {
		key1 = key2 = 0;
		keys = twl4030_keypad_keys_pressed(&key1, &key2);
		if ((1 == CFG_FASTBOOT_PREBOOT_KEYS) &&
		    (1 == keys)) {
			if (CFG_FASTBOOT_PREBOOT_KEY1 == key1)
				return 1;
		} else if ((2 == CFG_FASTBOOT_PREBOOT_KEYS) &&
			   (2 == keys)) {
			if ((CFG_FASTBOOT_PREBOOT_KEY1 == key1) &&
			    (CFG_FASTBOOT_PREBOOT_KEY2 == key2))
				return 1;
		}
		udelay(CFG_FASTBOOT_PREBOOT_LOOP_WAIT);
	}
#endif
	return 0;
}

int fastboot_init(struct cmd_fastboot_interface *interface) 
{
	int ret = 1;
	u8 devctl;

	device_strings[DEVICE_STRING_MANUFACTURER_INDEX]  = "Texas Instruments";
#if defined (CONFIG_3430ZOOM2)
	device_strings[DEVICE_STRING_PRODUCT_INDEX]       = "Zoom2";
#elif defined (CONFIG_3430LABRADOR)
	device_strings[DEVICE_STRING_PRODUCT_INDEX]       = "Zoom";
#else
	/* Default, An error message to prompt user */
#error "Need a product name for fastboot"

#endif
	/* These are just made up */
	device_strings[DEVICE_STRING_SERIAL_NUMBER_INDEX] = "00123";
	device_strings[DEVICE_STRING_CONFIG_INDEX]        = "Android Fastboot";
	device_strings[DEVICE_STRING_INTERFACE_INDEX]     = "Android Fastboot";

	/* The interface structure */
	fastboot_interface = interface;
	fastboot_interface->product_name                  = device_strings[DEVICE_STRING_PRODUCT_INDEX];
	fastboot_interface->serial_no                     = device_strings[DEVICE_STRING_SERIAL_NUMBER_INDEX];
	fastboot_interface->nand_block_size               = 2048;
	fastboot_interface->transfer_buffer               = (unsigned char *) CFG_FASTBOOT_TRANSFER_BUFFER;
	fastboot_interface->transfer_buffer_size          = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE;

	fastboot_reset();

	/* Check if device is in b-peripheral mode */
	devctl = inb (OMAP34XX_USB_DEVCTL);
	if (!(devctl & MUSB_DEVCTL_BDEVICE) ||
	    (devctl & MUSB_DEVCTL_HM)) 
	{
		printf ("ERROR : Unsupport USB mode\n");
		printf ("Check that mini-B USB cable is attached to the device\n");
	}
	else
	{
		ret = 0;
	}
	return ret;
}

#endif /* CONFIG_FASTBOOT */
