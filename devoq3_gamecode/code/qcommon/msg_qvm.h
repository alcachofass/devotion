/*
===========================================================================
Minimal message/Huffman API for QVM modules (ui.qvm) without full qcommon.h.
===========================================================================
*/
#ifndef MSG_QVM_H
#define MSG_QVM_H

#define	MAX_MSGLEN				16384

typedef struct {
	qboolean	allowoverflow;
	qboolean	overflowed;
	qboolean	oob;
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	int		bit;
} msg_t;

enum svc_ops_e {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,
	svc_baseline,
	svc_serverCommand,
	svc_download,
	svc_snapshot,
	svc_EOF,
	svc_extension,
	svc_voip
};

void MSG_Init( msg_t *buf, byte *data, int length );
void MSG_Bitstream( msg_t *buf );

int		MSG_ReadBits( msg_t *msg, int bits );
int		MSG_ReadChar( msg_t *sb );
int		MSG_ReadByte( msg_t *sb );
int		MSG_ReadShort( msg_t *sb );
int		MSG_ReadLong( msg_t *sb );
char	*MSG_ReadString( msg_t *sb );
char	*MSG_ReadBigString( msg_t *sb );
void	MSG_ReadData( msg_t *sb, void *buffer, int size );

#define NYT HMAX
#define INTERNAL_NODE (HMAX+1)
#define HMAX 256

typedef struct nodetype {
	struct	nodetype *left, *right, *parent;
	struct	nodetype *next, *prev;
	struct	nodetype **head;
	int		weight;
	int		symbol;
} node_t;

typedef struct {
	int			blocNode;
	int			blocPtrs;
	node_t*		tree;
	node_t*		lhead;
	node_t*		ltail;
	node_t*		loc[HMAX+1];
	node_t**	freelist;
	node_t		nodeList[768];
	node_t*		nodePtrs[768];
} huff_t;

typedef struct {
	huff_t		compressor;
	huff_t		decompressor;
} huffman_t;

void	Huff_Init( huffman_t *huff );
void	Huff_addRef( huff_t *huff, byte ch );
int		Huff_Receive( node_t *node, int *ch, byte *fin );
void	Huff_offsetReceive( node_t *node, int *ch, byte *fin, int *offset );
void	Huff_putBit( int bit, byte *fout, int *offset );
int		Huff_getBit( byte *fin, int *offset );

#endif /* MSG_QVM_H */
