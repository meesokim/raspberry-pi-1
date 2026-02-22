/*---------------------------------------------------------------------------/
/  Configurations of FatFs Module
/---------------------------------------------------------------------------*/

#define FFCONF_DEF	80386	/* Revision ID */

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/

#define FF_FS_READONLY	0
/* This option switches read-only configuration. (0:Read/Write or 1:Read-only) */

#define FF_FS_MINIMIZE	0
/* This option defines minimization level to remove some basic API functions. */

#define FF_USE_FIND		1
/* This option switches filtered directory read functions. */

#define FF_USE_MKFS		0
/* This option switches f_mkfs(). */

#define FF_USE_FASTSEEK	0
/* This option switches fast seek feature. */

#define FF_USE_EXPAND	0
/* This option switches f_expand(). */

#define FF_USE_CHMOD	0
/* This option switches attribute control API functions. */

#define FF_USE_LABEL	1
/* This option switches volume label API functions. */

#define FF_USE_FORWARD	0
/* This option switches f_forward(). */

#define FF_USE_STRFUNC	0
#define FF_PRINT_LLI	0
#define FF_PRINT_FLOAT	0
#define FF_STRF_ENCODE	0
/* FF_USE_STRFUNC switches string API functions. */


/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

#define FF_CODE_PAGE	437
/* This option specifies the OEM code page (Central Europe). */

#define FF_USE_LFN		3
#define FF_MAX_LFN		255
/* The FF_USE_LFN switches the support for LFN (long file name). */

#define FF_LFN_UNICODE	0
/* This option switches the character encoding on the API (0:ANSI/OEM). */

#define FF_LFN_BUF		255
#define FF_SFN_BUF		12

#define FF_FS_RPATH		2
/* This option configures support for relative path feature. (2:f_getcwd available) */

#define FF_PATH_DEPTH	10


/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/---------------------------------------------------------------------------*/

#define FF_VOLUMES		5
/* Number of volumes (logical drives) to be used. */

#define FF_STR_VOLUME_ID	1
#define FF_VOLUME_STRS		"SD","USB0","USB1","USB2","USB3"

#define FF_MULTI_PARTITION	0
/* This option switches support for multiple volumes on the physical drive. */

#define FF_MIN_SS		512
#define FF_MAX_SS		4096
/* This set of options configures the range of sector size to be supported. */

#define FF_LBA64		0
/* This option switches support for 64-bit LBA. (0:Disable) */

#define FF_MIN_GPT		0x10000000

#define FF_USE_TRIM		0


/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

#define FF_FS_TINY		0
/* This option switches tiny buffer configuration. */

#define FF_FS_EXFAT		0
/* This option switches support for exFAT filesystem. */

#define FF_FS_NORTC		0
#define FF_NORTC_MON	11
#define FF_NORTC_MDAY	9
#define FF_NORTC_YEAR	2014

#define FF_FS_CRTIME	0

#define FF_FS_NOFSINFO	0

#define FF_FS_LOCK		0

#define FF_FS_REENTRANT	0
#define FF_FS_TIMEOUT	1000


/*--- End of configuration options ---*/
