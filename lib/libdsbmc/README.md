
# NAME

**libdsbmc** - Client library for DSBMD

# SYNOPSIS

**#include &lt;libdsbmc.h>**

*int*  
**dsbmc\_fetch\_event**(*dsbmc\_t \*handle*, *dsbmc\_event\_t \*ev*);

*int*  
**dsbmc\_get\_devlist**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*\*\*list*);

*int*  
**dsbmc\_mount**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*);

*int*  
**dsbmc\_unmount**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *bool force*);

*int*  
**dsbmc\_eject**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *bool force*);

*int*  
**dsbmc\_set\_speed**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *int speed*);

*int*  
**dsbmc\_size**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*);

*int*  
**dsbmc\_mdattach**(*dsbmc\_t \*handle*, *const char \*image*);

*int*  
**dsbmc\_set\_speed\_async**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *int speed*, *void (\*cb)(int, const dsbmc\_dev\_t \*)*);

*int*  
**dsbmc\_mount\_async**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *void (\*cb)(int, const dsbmc\_dev\_t \*)*);

*int*  
**dsbmc\_unmount\_async**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *bool force*, *void (\*cb)(int, const dsbmc\_dev\_t \*)*);

*int*  
**dsbmc\_eject\_async**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *bool force*, *void (\*cb)(int, const dsbmc\_dev\_t \*)*);

*int*  
**dsbmc\_size\_async**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*d*, *void (\*cb)(int, const dsbmc\_dev\_t \*)*);

*int*  
**dsbmc\_mdattach\_async**(*dsbmc\_t \*handle*, *const char \*image*, *void (\*cb)(int, const dsbmc\_dev\_t \*)*);

*int*  
**dsbmc\_connect**(*dsbmc\_t \*handle*);

*int*  
**dsbmc\_get\_fd**(*dsbmc\_t \*handle*);

*int*  
**dsbmc\_get\_err**(*dsbmc\_t \*handle*, *const char \*\*buf*);

*void*  
**dsbmc\_disconnect**(*dsbmc\_t \*handle*);

*void*  
**dsbmc\_free\_handle**(*dsbmc\_t \*handle*);

*void*  
**dsbmc\_free\_dev**(*dsbmc\_t \*handle*, *const dsbmc\_dev\_t \*dev*);

*const char \*&zwnj;*  
**dsbmc\_errstr**(*dsbmc\_t \*handle*);

*const char \*&zwnj;*  
**dsbmc\_errcode\_to\_str**(*int code*);

*dsbmc\_t \*&zwnj;*  
**dsbmc\_alloc\_handle**(*void*);

*dsbmc\_dev\_t \*&zwnj;*  
**dsbmc\_dev\_from\_name**(*dsbmc\_t \*handle*, *const char \*name*);

*dsbmc\_dev\_t \*&zwnj;*  
**dsbmc\_dev\_from\_id**(*dsbmc\_t \*handle*, *int id*);

*dsbmc\_dev\_t \*&zwnj;*  
**dsbmc\_next\_dev**(*dsbmc\_t \*handle*, *int \*idx*, *bool removed*);

# DESCRIPTION

Almost all functions operate on a handle of type
*dsbmc\_t*.
**dsbmc\_alloc\_handle**()
creates and initializes a handle, and returns a pointer to it. If an error
occured, NULL is returned.

**dsbmc\_connect**()
establishes a connection to DSBMD, and returns 0 on success, and -1 if
an error occured. If this function fails, DSBMC\_ERR\_CONN\_DENIED is set in the
error bitmask, or errno is set to any value defined in connect(2) and
socket(2).

The
**dsbmc\_disconnect**()
function terminates the connection to DSBMD.

**dsbmc\_free\_handle**()
frees the memory used by the handle.

The
**dsbmc\_fetch\_event**()
function fetches the next available event, updates
internal data structures, and executes callback functions. If an event is
available,
**dsbmc\_fetch\_event**()
returns a value &gt; 0, else 0 is returned.
If an error occured,
**dsbmc\_fetch\_event**()
returns -1. Possible errors
are
`DSBMC_ERR_LOST_CONNECTION`, `DSBMC_ERR_SYS`
and/or
`DSBMC_ERR_FATAL`.
Instead of CPU intensive polling,
**dsbmc\_fetch\_event**()
can be used together
with select() by passing the file descriptor retrieved by
**dsbmd\_get\_fd**().

The
**dsbmc\_get\_devlist**()
fucntion sets the given pointer to the internal list
of
*dsbmc\_dev\_t \*&zwnj;*
objects. The number of objects is returned.

The
**dsbmc\_get\_fd**()
function returns the file descriptor of the socket which
is connected to DSBMD. Its ONLY purpose is to use it with select().

The
**dsbmc\_mount**()
function mounts the device represented by the given object.
The function blocks until a reply from DSBMD is received or an error occured.
On success
**dsbmc\_mount**()
returns 0. If mounting failed, an error code &gt; 0
is returned, which can be one of
`DSBMC_ERR_ALREADY_MOUNTED, DSBMC_ERR_NO_MEDIA DSBMC_ERR_PERMISSION_DENIED`,
`DSBMC_ERR_DEVICE_BUSY`, `DSBMC_ERR_NO_SUCH_DEVICE`,
`DSBMC_ERR_UNKNOWN_FILESYSTEM`, `DSBMC_ERR_UNKNOWN_ERROR`,
`DSBMC_ERR_MNTCMD_FAILED`.
If an error occured, -1 is returned, and the error bitmask is set accordingly.

The
**dsbmc\_unmount**()
function unmounts the device represented by the given
object. If 'force' is true, unmounting will be enforced even if the device is
busy. The function blocks until a reply from DSBMD is received or an error
occured. On success
**dsbmc\_unmount**()
returns 0. If unmounting failed, an error
code &gt; 0 is returned, which can be
`DSBMC_ERR_NOT_MOUNTED`, `DSBMC_ERR_DEVICE_BUSY`,
or an errno number. If an error occured, -1 is returned,
and the error bitmask is set accordingly.

The
**dsbmc\_eject**()
function ejects the device represented by the given object.
If 'force' is true, ejecting will be enforced even if the device is busy.
The function blocks until a reply from DSBMD is received or an error occured.
On success
**dsbmc\_eject**()
returns 0. If ejecting failed, an error code &gt; 0
is returned, which can be
`DSBMC_ERR_NOT_EJECTABLE`, `DSBMC_ERR_DEVICE_BUSY`,
or an errno number. If an error occured, -1 is returned, and the error
bitmask is set accordingly.

The
**dsbmc\_set\_speed**()
function sets the reading speed of the optical device
represented by the given object. The function blocks until a reply from DSBMD
is received or an error occured. On success
**dsbmc\_set\_speed**()
returns 0. If it failed, an error code &gt; 0 is returned, which can be
`ERR_INVALID_ARGUMENT`,
or an errno number. If an error occured, -1 is returned, and the error
bitmask is set accordingly.

The
**dsbmc\_size**()
function queries the storage capacity, the number of used and
free bytes of the device represented by the given object.

The
**dsbmc\_mdattach**()
function asks DSBMD to create a memory disk to access the given disk image.
The function blocks until a reply from DSBMD is received or an error occured.
On success
**dsbmc\_mdattach**()
returns 0, else an error code &gt; 0
is returned, which can be
`DSBMC_ERR_NOT_A_FILE`, `DSBMC_ERR_PERMISSION_DENIED`,
or an errno number. If an error occured, -1 is returned, and the error
bitmask is set accordingly.

The
**\_async**()
variant of
**dsbmc\_mount**(),
**dsbmc\_unmount**(),
**dsbmc\_eject**(),
**dsbmc\_set\_speed**(),
and
**dsbmc\_mdattach**()
return 0, and the given callback function will be executed by
**dsbmd\_fetch\_event**()
as soon as a command-reply code is received. If an error occured, -1 is
returned, and the error bitmask is set accordingly.

If the
`DSBMC_EVENT_DEL_DEVICE`
event was received, the device object should be removed from the list using
**dsbmc\_free\_dev**()
if it is no longer needed.

If one of the functions failed with an error, the function
**dsbmc\_get\_err**()
can be used to retrieve the error bitmask. If errstr is not
`NULL`,
it is set to the corresponding error message.

The function
**dsbmc\_errstr**()
returns the last error message.

The
**dsbmc\_errcode\_to\_str**()
function translates a DSBMD error-code into a string.

The
**dsbmc\_dev\_from\_name**()
function returns a pointer to the device object matching the given device name,
or NULL if there was no match.

The
**dsbmc\_dev\_from\_id**()
function returns a pointer to the device object matching the given ID,
or NULL if there was no match.

The
**dsbmc\_next\_dev**()
function returns the next device from the device list, starting from
*\*idx*,
and increases the content of the given index pointer. If
*removed*
is false, removed devices are ignored. If the end of the list is reached, NULL
is returned.

# THREADS

**libdsbmc**
can be used together with threads. However, access to a single handle from
multiple threads must be synchronized, e.g. by using a mutex.

# OBJECTS

Device object definition:

	typedef struct dsbmc_dev_s {
		uint8_t cmds;			/* Supported commands. */
	#define DSBMC_CMD_MOUNT			(1 << 0x00)
	#define DSBMC_CMD_UNMOUNT		(1 << 0x01)
	#define DSBMC_CMD_EJECT			(1 << 0x02)
	#define DSBMC_CMD_PLAY			(1 << 0x03)
	#define DSBMC_CMD_OPEN			(1 << 0x04)
	#define DSBMC_CMD_SPEED			(1 << 0x05)
	#define DSBMC_CMD_SIZE			(1 << 0x06)
	#define DSBMC_CMD_MDATTACH		(1 << 0x07)
		uint8_t type;
	#define DSBMC_DT_HDD			0x01
	#define DSBMC_DT_USBDISK		0x02
	#define DSBMC_DT_DATACD			0x03
	#define	DSBMC_DT_AUDIOCD		0x04
	#define	DSBMC_DT_RAWCD			0x05
	#define	DSBMC_DT_DVD			0x06
	#define	DSBMC_DT_VCD			0x07
	#define	DSBMC_DT_SVCD			0x08
	#define	DSBMC_DT_FLOPPY			0x09
	#define DSBMC_DT_MMC			0x0a
	#define DSBMC_DT_MTP			0x0b
	#define DSBMC_DT_PTP			0x0c
		int	 id;			/* Unique ID */
		char	 *dev;			/* Device name */
		char	 *volid;		/* Volume ID */
		char	 *mntpt;		/* Mount point */
		char	 *fsname;		/* Filesystem name */
		bool	 mounted;		/* Whether drive is mounted. */
		bool	 removed;		/* Whether device was removed */
		uint8_t	 speed;			/* CD/DVD reading speed */
		uint64_t mediasize;		/* For "size" command. */
		uint64_t free;			/* 	 ""	       */
		uint64_t used;			/* 	 ""	       */
	} dsbmc_dev_t;

Event object definition:

	typedef struct dsbmc_event_s {
		int type;			/* Event type */
		int code;			/* Error code */
		dsbmc_dev_t *dev;		/* Device object */
	} dsbmc_event_t;

# ERROR CODES

**dsbmc\_get\_err**()
returns the following error codes as bitmask:

`DSBMC_ERR_SYS`

> Syscall failed

`DSBMC_ERR_FATAL`

> Fatal error. Continuing the program is probably not possible.

`DSBMC_ERR_LOST_CONNECTION`

> Connection to DSBMD lost.

`DSBMC_ERR_CONN_DENIED`

> The client has no permission to connect to DSBMD.

`DSBMC_ERR_INVALID_DEVICE`

> A NULL pointer or a pointer to a removed device object was passed.

`DSBMC_ERR_CMDQ_BUSY`

> The max. number of commands to be send asynchronously was exceeded.

`DSBMC_ERR_COMMAND_IN_PROGRESS`

> An attempt was made to send a command while there are still commands
> in the command queue.

Possible error codes of dsbmd events are:

`DSBMC_ERR_ALREADY_MOUNTED, DSBMC_ERR_PERMISSION_DENIED, DSBMC_ERR_NOT_MOUNTED`, `DSBMC_ERR_DEVICE_BUSY`, `DSBMC_ERR_NO_SUCH_DEVICE`, `DSBMC_ERR_MAX_CONN_REACHED`, `DSBMC_ERR_NOT_EJECTABLE`, `DSBMC_ERR_UNKNOWN_COMMAND`, `DSBMC_ERR_UNKNOWN_OPTION`, `DSBMC_ERR_SYNTAX_ERROR`, `DSBMC_ERR_NO_MEDIA`, `DSBMC_ERR_UNKNOWN_FILESYSTEM`, `DSBMC_ERR_UNKNOWN_ERROR`, `DSBMC_ERR_MNTCMD_FAILED, DSBMC_ERR_NOT_A_FILE`

