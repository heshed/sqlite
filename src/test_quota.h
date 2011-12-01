/*
** 2011 December 1
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file contains the interface definition for the quota a VFS shim.
**
** This particular shim enforces a quota system on files.  One or more
** database files are in a "quota group" that is defined by a GLOB
** pattern.  A quota is set for the combined size of all files in the
** the group.  A quota of zero means "no limit".  If the total size
** of all files in the quota group is greater than the limit, then
** write requests that attempt to enlarge a file fail with SQLITE_FULL.
**
** However, before returning SQLITE_FULL, the write requests invoke
** a callback function that is configurable for each quota group.
** This callback has the opportunity to enlarge the quota.  If the
** callback does enlarge the quota such that the total size of all
** files within the group is less than the new quota, then the write
** continues as if nothing had happened.
*/
#ifndef _QUOTA_H_
#include "sqlite3.h"
#include <stdio.h>

/*
** Initialize the quota VFS shim.  Use the VFS named zOrigVfsName
** as the VFS that does the actual work.  Use the default if
** zOrigVfsName==NULL.  
**
** The quota VFS shim is named "quota".  It will become the default
** VFS if makeDefault is non-zero.
**
** THIS ROUTINE IS NOT THREADSAFE.  Call this routine exactly once
** during start-up.
*/
int sqlite3_quota_initialize(const char *zOrigVfsName, int makeDefault);

/*
** Shutdown the quota system.
**
** All SQLite database connections must be closed before calling this
** routine.
**
** THIS ROUTINE IS NOT THREADSAFE.  Call this routine exactly once while
** shutting down in order to free all remaining quota groups.
*/
int sqlite3_quota_shutdown(void);

/*
** Create or destroy a quota group.
**
** The quota group is defined by the zPattern.  When calling this routine
** with a zPattern for a quota group that already exists, this routine
** merely updates the iLimit, xCallback, and pArg values for that quota
** group.  If zPattern is new, then a new quota group is created.
**
** The zPattern is always compared against the full pathname of the file.
** Even if APIs are called with relative pathnames, SQLite converts the
** name to a full pathname before comparing it against zPattern.  zPattern
** is a standard glob pattern with the following matching rules:
**
**      '*'       Matches any sequence of zero or more characters.
**
**      '?'       Matches exactly one character.
**
**     [...]      Matches one character from the enclosed list of
**                characters.
**
**     [^...]     Matches one character not in the enclosed list.
**
** Note that, unlike unix shell globbing, the directory separator "/"
** can match a wildcard.  So, for example, the pattern "/abc/xyz/" "*"
** matches any files anywhere in the directory hierarchy beneath
** /abc/xyz
**
** If the iLimit for a quota group is set to zero, then the quota group
** is disabled and will be deleted when the last database connection using
** the quota group is closed.
**
** Calling this routine on a zPattern that does not exist and with a
** zero iLimit is a no-op.
**
** A quota group must exist with a non-zero iLimit prior to opening
** database connections if those connections are to participate in the
** quota group.  Creating a quota group does not affect database connections
** that are already open.
*/
int sqlite3_quota_set(
  const char *zPattern,           /* The filename pattern */
  sqlite3_int64 iLimit,           /* New quota to set for this quota group */
  void (*xCallback)(              /* Callback invoked when going over quota */
     const char *zFilename,         /* Name of file whose size increases */
     sqlite3_int64 *piLimit,        /* IN/OUT: The current limit */
     sqlite3_int64 iSize,           /* Total size of all files in the group */
     void *pArg                     /* Client data */
  ),
  void *pArg,                     /* client data passed thru to callback */
  void (*xDestroy)(void*)         /* Optional destructor for pArg */
);

/*
** Bring the named file under quota management.  Or if it is already under
** management, update its size.
*/
int sqlite3_quota_file(const char *zFilename);

/*
** The following object serves the same role as FILE in the standard C
** library.  It represents an open connection to a file on disk for I/O.
*/
typedef struct quota_FILE quota_FILE;

/*
** Create a new quota_FILE object used to read and/or write to the
** file zFilename.  The zMode parameter is as with standard library zMode.
*/
quota_FILE *sqlite3_quota_fopen(const char *zFilename, const char *zMode);

/*
** Perform I/O against a quota_FILE object.  When doing writes, the
** quota mechanism may result in a short write, in order to prevent
** the sum of sizes of all files from going over quota.
*/
size_t sqlite3_quota_fread(void*, size_t, size_t, quota_FILE*);
size_t sqlite3_quota_fwrite(void*, size_t, size_t, quota_FILE*);

/*
** Close a quota_FILE object and free all associated resources.  The
** file remains under quota management.
*/
int sqlite3_quota_fclose(quota_FILE*);

/*
** Move the read/write pointer for a quota_FILE object.  Or tell the
** current location of the read/write pointer.
*/
int sqlite3_quota_fseek(quota_FILE*, long, int);
void sqlite3_quota_rewind(quota_FILE*);
long sqlite3_quota_ftell(quota_FILE*);

/*
** Delete a file from the disk.  If that file is under quota management,
** then adjust quotas accordingly.
**
** The file being deleted must not be open for reading or writing or as
** a database when it is deleted.
*/
int sqlite3_quota_remove(const char *zFilename);

#endif /* _QUOTA_H_ */