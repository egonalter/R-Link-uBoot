/*
 * (C) Copyright 2008
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
 *
 * Part of the rx_handler were copied from the Android project. 
 * Specifically rx command parsing in the  usb_rx_data_complete 
 * function of the file bootable/bootloader/legacy/usbloader/usbloader.c
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_* 
 * They come from bootable/bootloader/legacy/libboot/flash.c
 *
 * This is their Copyright:
 * 
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <nand.h>
#include <fastboot.h>

#if (CONFIG_FASTBOOT)

/* Use do_reset for fastboot's 'reboot' command */
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
/* Use do_nand for fastboot's flash commands */
extern int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
/* Use do_setenv and do_saveenv to permenantly save data */
int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
/* Use do_bootm and do_go for fastboot's 'boot' command */
int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

/* Forward decl */
static int rx_handler (const unsigned char *buffer, unsigned int buffer_size);
static void reset_handler (void);

static struct cmd_fastboot_interface interface = 
{
	.rx_handler            = rx_handler,
	.reset_handler         = reset_handler,
	.product_name          = NULL,
	.serial_no             = NULL,
	.nand_block_size       = 0,
	.transfer_buffer       = (unsigned char *)0xffffffff,
	.transfer_buffer_size  = 0,
};

static unsigned int download_size = 0;
static unsigned int download_bytes = 0;

static void save_contiguous_block_values (struct fastboot_ptentry *ptn, 
					  unsigned int offset, 
					  unsigned int size)
{
	struct fastboot_ptentry *env_ptn;

	char var[64], val[32];
	char start[32], length[32];
	char ecc_type[32];

	char *lock[5]    = { "nand", "lock",   NULL, NULL, NULL, };
	char *unlock[5]  = { "nand", "unlock", NULL, NULL, NULL, };
	char *ecc[4]     = { "nand", "ecc",    NULL, NULL, };	
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };
	char *saveenv[2] = { "setenv", NULL, };
	
	setenv[1] = var;
	setenv[2] = val;
	lock[2] = unlock[2] = start;
	lock[3] = unlock[3] = length;

	printf ("saving it..\n");

	if (size == 0)
	{
		/* The error case, where the variables are being unset */
		
		sprintf (var, "%s_nand_offset", ptn->name);
		sprintf (val, "");
		do_setenv (NULL, 0, 3, setenv);

		sprintf (var, "%s_nand_size", ptn->name);
		sprintf (val, "");
		do_setenv (NULL, 0, 3, setenv);
	}
	else
	{
		/* Normal case */

		sprintf (var, "%s_nand_offset", ptn->name);
		sprintf (val, "0x%x", offset);

		printf ("%s %s %s\n", setenv[0], setenv[1], setenv[2]);
		
		do_setenv (NULL, 0, 3, setenv);

		sprintf (var, "%s_nand_size", ptn->name);

		sprintf (val, "0x%x", size);

		printf ("%s %s %s\n", setenv[0], setenv[1], setenv[2]);

		do_setenv (NULL, 0, 3, setenv);
	}


	/* Warning : 
	   The environment is assumed to be in a partition named 'enviroment'.
	   It is very possible that your board stores the enviroment 
	   someplace else. */
	env_ptn = fastboot_flash_find_ptn("environment");

	if (env_ptn)
	{
		/* Some flashing requires the nand's ecc to be set */
		ecc[2] = ecc_type;
		if ((env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC) &&
		    (env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC)) 
		{
			/* Both can not be true */
			printf ("Warning can not do hw and sw ecc for partition '%s'\n", ptn->name);
			printf ("Ignoring these flags\n");
		} 
		else if (env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC)
		{
			sprintf (ecc_type, "hw");
			do_nand (NULL, 0, 3, ecc);
		}
		else if (env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC)
		{
			sprintf (ecc_type, "sw");
			do_nand (NULL, 0, 3, ecc);
		}
		
		sprintf (start, "0x%x", env_ptn->start);
		sprintf (length, "0x%x", env_ptn->length);			

		/* This could be a problem is there is an outstanding lock */
		do_nand (NULL, 0, 4, unlock);
	}

	do_saveenv (NULL, 0, 1, saveenv);
	
	if (env_ptn)
	{
		do_nand (NULL, 0, 4, lock);
	}
}

static void reset_handler ()
{
	/* If there was a download going on, bail */
	download_size = 0;
	download_bytes = 0;
}

static int rx_handler (const unsigned char *buffer, unsigned int buffer_size)
{
	int ret = 1;

	/* Use 65 instead of 64
	   null gets dropped  
	   strcpy's need the extra byte */
	char response[65];

	if (download_size) 
	{
		/* Something to download */

		if (buffer_size)
		{
			/* Handle possible overflow */
			unsigned int transfer_size = 
				download_size - download_bytes;

			if (buffer_size < transfer_size)
				transfer_size = buffer_size;
			
			/* Save the data to the transfer buffer */
			memcpy (interface.transfer_buffer + download_bytes, 
				buffer, transfer_size);

			download_bytes += transfer_size;
			
			/* Check if transfer is done */
			if (download_bytes >= download_size)
			{
				/* Reset global transfer variable, 
				   Keep download_bytes because it will be 
				   used in the next possible flashing command */
				download_size = 0;

				/* Everything has transferred, 
				   send the OK response */
				sprintf(response,"OKAY");
				fastboot_tx_status(response, strlen(response));

				printf ("\ndownloading of %d bytes finished\n",
					download_bytes);

				/* Pad the download to be nand block aligned */
				if (interface.nand_block_size)
				{
					if (download_bytes % 
					    interface.nand_block_size)
					{
						unsigned int pad = interface.nand_block_size - (download_bytes % interface.nand_block_size);
						unsigned int i;
						
						for (i = 0; i < pad; i++) 
						{
							if (download_bytes >= interface.transfer_buffer_size)
								break;
							
							interface.transfer_buffer[download_bytes] = 0;
							download_bytes++;
						}
					}
				}
			}
			else if (download_bytes && 
				 0 == (download_bytes % (16 * interface.nand_block_size)))
			{
				/* Some feeback that the download is happening */
				printf (".");
				if (0 == (download_bytes % (80 * 16 * interface.nand_block_size)))
					printf ("\n");
				
			}
			
		}
		else
		{
			/* Ignore empty buffers */
			printf ("Warning empty download buffer\n");
			printf ("Ignoring\n");
		}
		ret = 0;
	}
	else
	{
		/* A command */

		/* Cast to make compiler happy with string functions */
		const char *cmdbuf = (char *) buffer;

		/* Generic failed response */
		sprintf(response, "FAIL");

		/* reboot 
		   Reboot the board. */

		if(memcmp(cmdbuf, "reboot", 6) == 0) 
		{
			sprintf(response,"OKAY");
			fastboot_tx_status(response, strlen(response));
			udelay (1000000); /* 1 sec */
			
			do_reset (NULL, 0, 0, NULL);
			
			/* This code is unreachable,
			   leave it to make the compiler happy */
			return 0;
		}
		
		/* getvar
		   Get common fastboot variables
		   Board has a chance to handle other variables */
		if(memcmp(cmdbuf, "getvar:", 7) == 0) 
		{
			strcpy(response,"OKAY");
        
			if(!strcmp(cmdbuf + strlen("version"), "version")) 
			{
				strcpy(response + 4, FASTBOOT_VERSION);
			} 
			else if(!strcmp(cmdbuf + strlen("product"), "product")) 
			{
				if (interface.product_name) 
					strcpy(response + 4, interface.product_name);
			
			} else if(!strcmp(cmdbuf + strlen("serialno"), "serialno")) {
				if (interface.serial_no) 
					strcpy(response + 4, interface.serial_no);

			} else if(!strcmp(cmdbuf + strlen("downloadsize"), "downloadsize")) {
				if (interface.transfer_buffer_size) 
					sprintf(response + 4, "08x", interface.transfer_buffer_size);
			} 
			else 
			{
				fastboot_getvar(cmdbuf + 7, response + 4);
			}
			ret = 0;

		}

		/* erase
		   Erase a register flash partition
		   Board has to set up flash partitions */

		if(memcmp(cmdbuf, "erase:", 6) == 0){
			struct fastboot_ptentry *ptn;

			ptn = fastboot_flash_find_ptn(cmdbuf + 6);
			if(ptn == 0) 
			{
				sprintf(response, "FAILpartition does not exist");
			}
			else
			{
				char start[32], length[32];
				int status, repeat, repeat_max;
			
				printf("erasing '%s'\n", ptn->name);   

				char *lock[5]   = { "nand", "lock",   NULL, NULL, NULL, };
				char *unlock[5] = { "nand", "unlock", NULL, NULL, NULL,	};
				char *erase[5]  = { "nand", "erase",  NULL, NULL, NULL, };
			
				lock[2] = unlock[2] = erase[2] = start;
				lock[3] = unlock[3] = erase[3] = length;

				repeat_max = 1;
				if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK)
					repeat_max = ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK;

				sprintf (length, "0x%x", ptn->length);
				for (repeat = 0; repeat < repeat_max; repeat++) 
				{
					sprintf (start, "0x%x", ptn->start + (repeat * ptn->length));
				
					do_nand (NULL, 0, 4, unlock);
					status = do_nand (NULL, 0, 4, erase);
					do_nand (NULL, 0, 4, lock);

					if (status)
						break;
				}

				if (status)
				{
					sprintf(response,"FAILfailed to erase partition");
				} 
				else 
				{
					printf("partition '%s' erased\n", ptn->name);
					sprintf(response, "OKAY");
				}
			
			}
			ret = 0;

		}

		/* download
		   download something .. 
		   What happens to it depends on the next command after data */

		if(memcmp(cmdbuf, "download:", 9) == 0) {

			/* save the size */
			download_size = simple_strtoul (cmdbuf + 9, NULL, 16);
			/* Reset the bytes count, now it is safe */
			download_bytes = 0;

			printf ("Starting download of %d bytes\n", download_size);

			if (0 == download_size)
			{
				/* bad user input */
				sprintf(response, "FAILdata invalid size");
			}
			else if (download_size > interface.transfer_buffer_size)
			{
				/* set download_size to 0 because this is an error */
				download_size = 0;
				sprintf(response, "FAILdata too large");
			}
			else
			{
				sprintf(response, "DATA%08x", download_size);
			}
			ret = 0;
		}

		/* boot 
		   boot what was downloaded 

		   WARNING WARNING WARNING

		   This is not what you expect. 
		   The fastboot client does its own packaging of the 
		   kernel.  The layout is defined in the android header
		   file bootimage.h.  This layeout is copiedlooks like this, 

		   **
		   ** +-----------------+ 
		   ** | boot header     | 1 page
		   ** +-----------------+
		   ** | kernel          | n pages  
		   ** +-----------------+
		   ** | ramdisk         | m pages  
		   ** +-----------------+
		   ** | second stage    | o pages
		   ** +-----------------+
		   **

		   We only care about the kernel. 
		   So we have to jump past a page. 

		   What is a page size ? 
		   The fastboot client uses 2048

		   The is the default value of 

		   CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE

		*/

		if(memcmp(cmdbuf, "boot", 4) == 0) {

			if ((download_bytes) &&
			    (CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE < download_bytes))
			{
				char start[32];
				char *bootm[3] = { "bootm", NULL, NULL, };
				char *go[3]    = { "go",    NULL, NULL, };

				/* Skip the mkbootimage header */
				image_header_t *hdr = 
					(image_header_t *) &interface.transfer_buffer[CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE];

				bootm[1] = go[1] = start;
				sprintf (start, "0x%x", hdr);

				/* Execution should jump to kernel so send the response
				   now and wait a bit.  */
				sprintf(response, "OKAY");
				fastboot_tx_status(response, strlen(response));
				udelay (1000000); /* 1 sec */

				if (ntohl(hdr->ih_magic) == IH_MAGIC) {
					/* Looks like a kernel.. */
					printf ("Booting kernel..\n");

					do_bootm (NULL, 0, 2, bootm);
				} else {
					/* Raw image, maybe another uboot */
					printf ("Booting raw image..\n");

					do_go (NULL, 0, 2, go);
				}
				printf ("ERROR : bootting failed\n");
				printf ("You should reset the board\n");
			} 
			sprintf(response, "FAILinvalid boot image");
			ret = 0;
		}
		

		/* flash 
		   Flash what was downloaded */

		if(memcmp(cmdbuf, "flash:", 6) == 0) {

			if (download_bytes) 
			{
				struct fastboot_ptentry *ptn;
			
				ptn = fastboot_flash_find_ptn(cmdbuf + 6);
				if(ptn == 0) 
				{
					sprintf(response, "FAILpartition does not exist");
				}
				else if (download_bytes > ptn->length)
				{
					sprintf(response, "FAILimage too large for partition");
				}
				else
				{
					/* inputs OK */

					char start[32], length[32];
					char wstart[32], wlength[32], addr[32];
					char ecc_type[32];
					int status, repeat, repeat_max;
			
					printf("flashing '%s'\n", ptn->name);   
					
					char *lock[5]   = { "nand", "lock",   NULL, NULL, NULL, };
					char *unlock[5] = { "nand", "unlock", NULL, NULL, NULL,	};
					char *write[6]  = { "nand", "write",  NULL, NULL, NULL, NULL, };
					char *ecc[4]    = { "nand", "ecc",    NULL, NULL, };
					char *erase[5]  = { "nand", "erase",  NULL, NULL, NULL, };

					lock[2] = unlock[2] = erase[2] = start;
					lock[3] = unlock[3] = erase[3] = length;

					write[2] = addr;				
					write[3] = wstart;
					write[4] = wlength;

					/* Some flashing requires the nand's ecc to be set */
					ecc[2] = ecc_type;
					if ((ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC) &&
					    (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC)) 
					{
						/* Both can not be true */
						printf ("Warning can not do hw and sw ecc for partition '%s'\n", ptn->name);
						printf ("Ignoring these flags\n");
					} 
					else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC)
					{
						sprintf (ecc_type, "hw");
						do_nand (NULL, 0, 3, ecc);
					}
					else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC)
					{
						sprintf (ecc_type, "sw");
						do_nand (NULL, 0, 3, ecc);
					}
					
					/* Some flashing requires writing the same data in multiple, consecutive flash partitions */
					repeat_max = 1;
					if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK)
					{
						if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK)
						{
							printf ("Warning can not do both 'contiguous block' and 'repeat' writes for for partition '%s'\n", ptn->name);
							printf ("Ignoring repeat flag\n");
						}
						else
						{
							repeat_max = ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK;
						}
					}
					
					/* Unlock the whole partition instead of trying to
					   manage special cases */
					sprintf (length, "0x%x", ptn->length);

					for (repeat = 0; repeat < repeat_max; repeat++) 

					{
						/* Poison status */
						status = 1;
						
						sprintf (start, "0x%x", ptn->start + (repeat * ptn->length));

						do_nand (NULL, 0, 4, unlock);
						do_nand (NULL, 0, 4, erase);

						if ((ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK) &&
						    (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK))
						{
							/* Both can not be true */
							printf ("Warning can not do 'next good block' and 'contiguous block' for partition '%s'\n", ptn->name);
							printf ("Ignoring these flags\n");
						}
						else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK)
						{
							/* Keep writing until you get a good block
							   transfer_buffer should already be aligned */
							if (interface.nand_block_size)
							{
								unsigned int blocks = download_bytes / interface.nand_block_size;
								unsigned int i = 0;

								unsigned int offset = 0;
								sprintf (wlength, "0x%x", interface.nand_block_size);
								while (i < blocks)
								{
									/* Check for overflow */
									if (offset >= ptn->length)
										break;
									
									/* download's address only advance if last write was successful */
									sprintf (addr,    "0x%x", interface.transfer_buffer + (i * interface.nand_block_size));

									/* nand's address always advances */
									sprintf (wstart,  "0x%x", ptn->start + (repeat * ptn->length) + offset);

									status = do_nand (NULL, 0, 5, write);
									if (status) 
										break;
									else
										i++;

									/* Go to next nand block */
									offset += interface.nand_block_size;
								}
							}
							else
							{
								printf ("Warning nand block size can not be 0 when using 'next good block' for partition '%s'\n", ptn->name);
								printf ("Ignoring write request\n");
							}
							
						}
						else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK)
						{
							/* Keep writing until you get a good block
							   transfer_buffer should already be aligned */
							if (interface.nand_block_size)
							{
								if (0 == nand_curr_device)
								{
									nand_info_t* nand;
									unsigned long off;
									unsigned int ok_start;

									nand = &nand_info[nand_curr_device];

									printf("\nDevice %d bad blocks:\n", nand_curr_device);
									
									/* Initialize the ok_start to the start of the partition
									   Then try to find a block large enough for the download */
									ok_start = ptn->start;
									
									/* It is assumed that the start and length are multiples of block size */
									for (off = ptn->start; off < ptn->start + ptn->length; off += nand->erasesize)
									{
										if (nand_block_isbad(nand, off))
										{
											/* Reset the ok_start to the next block */
											ok_start = off + nand->erasesize;
										}
										
										/* Check if we have enough blocks */
										if ((ok_start - off) >= download_bytes)
											break;
									}
									
									/* Check if there is enough space */
									if (ok_start + download_bytes <= ptn->start + ptn->length)
									{
										sprintf (addr,    "0x%x", interface.transfer_buffer);
										sprintf (wstart,  "0x%x", ok_start);
										sprintf (wlength, "0x%x", download_bytes);

										status = do_nand (NULL, 0, 5, write);

										/* Save the results into an environment variable on the format
										   ptn_name + 'offset'
										   ptn_name + 'size'  */
										if (status)
										{
											/* failed */
											save_contiguous_block_values (ptn, 0, 0);
										}
										else
										{
											/* success */
											save_contiguous_block_values (ptn, ok_start, download_bytes);
										}

									}
									else
									{
										printf ("Error could not find enough contiguous space in partition '%s' \n", ptn->name);
										printf ("Ignoring write request\n");
									}
								}
								else
								{
									/* TBD : Generalize flash handling */
									printf ("Error only handling 1 NAND per board");
									printf ("Ignoring write request\n");
								}
							}
							else
							{
								printf ("Warning nand block size can not be 0 when using 'continuous block' for partition '%s'\n", ptn->name);
								printf ("Ignoring write request\n");
							}
						}
						else 
						{
							/* Normal case */
							sprintf (addr,    "0x%x", interface.transfer_buffer);
							sprintf (wstart,  "0x%x", ptn->start + (repeat * ptn->length));
							sprintf (wlength, "0x%x", download_bytes);

							status = do_nand (NULL, 0, 5, write);
						}
						

						do_nand (NULL, 0, 4, lock);
						
						if (status)
							break;
					}
					
					if (status)
					{
						printf("flashing '%s' failed\n", ptn->name);
						sprintf(response,"FAILfailed to flash partition");
					} 
					else 
					{
						printf("partition '%s' flashed\n", ptn->name);
						sprintf(response, "OKAY");
					}
				}

			}
			else
			{
				sprintf(response, "FAILno image downloaded");
			}

			ret = 0;
		}

		fastboot_tx_status(response, strlen(response));

	} /* End of command */
	
	return ret;
}


	
int do_fastboot (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1;

	/* Initialize the board specific support */
	if (0 == fastboot_init(&interface))
	{
		printf ("Disconnect USB cable to finish fastboot..\n");
		
		/* If we got this far, we are a success */
		ret = 0;

		/* On disconnect or error, polling returns non zero */
		while (1)
		{
			if (fastboot_poll())
				break;
		}
	}

end:
	/* Reset the board specific support */
	fastboot_shutdown();
	
	return ret;
}

U_BOOT_CMD(
	fastboot,	1,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	NULL
);


/* To support the Android-style naming of flash */
#define MAX_PTN 16

static fastboot_ptentry ptable[MAX_PTN];
static unsigned int pcount = 0;

void fastboot_flash_add_ptn(fastboot_ptentry *ptn)
{
    if(pcount < MAX_PTN){
        memcpy(ptable + pcount, ptn, sizeof(*ptn));
        pcount++;
    }
}

void fastboot_flash_dump_ptn(void)
{
    unsigned int n;
    for(n = 0; n < pcount; n++) {
        fastboot_ptentry *ptn = ptable + n;
        printf("ptn %d name='%s' start=%d len=%d\n",
                n, ptn->name, ptn->start, ptn->length);
    }
}


fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
    unsigned int n;
    
    for(n = 0; n < pcount; n++) {
	    /* Make sure a substring is not accepted */
	    if (strlen(name) == strlen(ptable[n].name))
	    {
		    if(0 == strcmp(ptable[n].name, name))
			    return ptable + n;
	    }
    }
    return 0;
}

fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
    if(n < pcount) {
        return ptable + n;
    } else {
        return 0;
    }
}

unsigned int fastboot_flash_get_ptn_count(void)
{
    return pcount;
}



#endif	/* CONFIG_FASTBOOT */
