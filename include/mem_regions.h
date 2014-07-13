#ifndef __INCLUDE_MEM_REGIONS_H
#define __INCLUDE_MEM_REGIONS_H

/* All of these are defined in the <board>/u-boot.lds script */

#ifndef __ASSEMBLER__

extern unsigned long _end;

/* Region of memory reserved for FDT */
extern const int __fdt_start;
#define MEMADDR_FDT_START	((void *) &__fdt_start)
extern const int __fdt_end;
#define MEMADDR_FDT_END		((void *) &__fdt_end)
#define MEMADDR_FDT_LEN		((size_t) (MEMADDR_FDT_END -		\
					   MEMADDR_FDT_START))

/* Region of memory reserved for kernel */
extern const int __kern_start;
#define MEMADDR_KERN_START	((void *) &__kern_start)
extern const int __kern_end;
#define MEMADDR_KERN_END	((void *) &__kern_end)
#define MEMADDR_KERN_LEN	((size_t) (MEMADDR_KERN_END -		\
					   MEMADDR_KERN_START))

#endif

#endif /* __INCLUDE_MEM_REGIONS_H */
