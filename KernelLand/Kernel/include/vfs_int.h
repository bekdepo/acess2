/* 
 * Acess Micro - VFS Server Ver 1
 */
#ifndef _VFS_INT_H
#define _VFS_INT_H

#include "vfs.h"
#include <rwlock.h>

// === TYPES ===
typedef struct sVFS_Mount {
	struct sVFS_Mount	*Next;
	char	*MountPoint;
	size_t	MountPointLen;
	Uint32	Identifier;
	char	*Device;
	char	*Options;
	tVFS_Driver	*Filesystem;
	tVFS_Node	*RootNode;
	
	 int	OpenHandleCount;
	
	char	StrData[];
} tVFS_Mount;

typedef struct sVFS_Handle {
	tVFS_Node	*Node;
	tVFS_Mount	*Mount;
	Uint64	Position;
	Uint	Mode;
} tVFS_Handle;

typedef struct sVFS_Proc {
	struct sVFS_Proc	*Next;
	 int	ID;
	 int	CwdLen;
	Uint	UID, GID;
	char	*Cwd;
	 int	MaxHandles;
	tVFS_Handle	Handles[];
} tVFS_Proc;

typedef struct sVFS_MMapPage {
	Uint64	FileOffset;
	tPAddr	PAddr;
} tVFS_MMapPage;

// === GLOBALS ===
extern tRWLock  	glVFS_MountList;
extern tVFS_Mount	*gVFS_Mounts;
extern tVFS_Driver	*gVFS_Drivers;

// === PROTOTYPES ===
extern void	VFS_Deinit(void);
// --- open.c ---
extern char	*VFS_GetAbsPath(const char *Path);
extern tVFS_Node	*VFS_ParsePath(const char *Path, char **TruePath, tVFS_Mount **MountPoint);
extern tVFS_Handle	*VFS_GetHandle(int FD);
// --- acls.c ---
extern int	VFS_CheckACL(tVFS_Node *Node, Uint Permissions);
// --- mount.c ---
extern tVFS_Mount	*VFS_GetMountByIdent(Uint32 MountID);
// --- dir.c ---
extern int	VFS_MkNod(const char *Path, Uint Flags);
// --- handle.c ---
extern int	VFS_AllocHandle(int bIsUser, tVFS_Node *Node, int Mode);
extern int	VFS_SetHandle(int FD, tVFS_Node *Node, int Mode);


// --- VFS Helpers ---
static inline void _CloseNode(tVFS_Node *Node)
{
	if(Node && Node->Type && Node->Type->Close)
		Node->Type->Close( Node );
}
static inline void _ReferenceNode(tVFS_Node *Node)
{
	if( !MM_GetPhysAddr(Node->Type) ) {
		Log_Error("VFS", "Node %p's type is invalid (%p bad pointer) - %P corrupted",
			Node, Node->Type, MM_GetPhysAddr(&Node->Type));
		return ;
	}
	if( Node->Type && Node->Type->Reference )
		Node->Type->Reference( Node );
	else
		Node->ReferenceCount ++;
}


#endif
