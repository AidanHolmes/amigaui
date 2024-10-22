//   Copyright 2024 Aidan Holmes
//   IFF file parser

#ifndef __IFF_HDR
#define __IFF_HDR

#include <stdio.h>
#include <exec/types.h>
#include <proto/graphics.h>
#include <graphics/view.h>
#include <intuition/intuition.h>

#define IFF_NO_ERROR			0
#define IFF_STREAM_ERROR		1
#define IFF_NOIMAGE_ERROR		2
#define IFF_NODATA_ERROR		3
#define IFF_COMPLEX_ERROR		4
#define IFF_PARSE_ERROR			5
#define IFF_NOTIFF_ERROR		3
#define IFF_ABORT_NO_ERROR		6
#define IFF_NORESOURCE_ERROR	7
#define IFF_NORTG_ERROR			8

#define IFF_BITPLANE		0x01
#define IFF_CHUNKY			0x02

#define IFF_STATE_NEW_CHUNK		1
#define IFF_STATE_CHUNK_LEN		2
#define IFF_STATE_ID_CHUNK		3
#define IFF_STATE_DATA			4
#define IFF_STATE_DATA_LEN		5
#define IFF_STATE_SKIP			6

#define IFF_TYPE_NONE			0
#define IFF_TYPE_FORM			1
#define IFF_TYPE_LIST			2
#define IFF_TYPE_CAT			3
#define IFF_TYPE_PROP			4
#define IFF_TYPE_BMHD			5
#define IFF_TYPE_CMAP			6
#define IFF_TYPE_BODY			7
#define IFF_TYPE_CAMG			8

#define IFF_MAX_DEPTH			6
#define IFF_BUFF_LEN			128
#define IFF_ID_LEN				4

struct _BitmapHeader{
	/* Chunk data starts here */
	UWORD  Width;        /* Width of image in pixels */
	UWORD  Height;       /* Height of image in pixels */
	UWORD  Left;         /* X coordinate of image */
	UWORD  Top;          /* Y coordinate of image */
	UBYTE  Bitplanes;    /* Number of bitplanes */
	UBYTE  Masking;      /* Type of masking used */
	UBYTE  Compress;     /* Compression method use on image data */
	UBYTE  Padding;      /* Alignment padding (always 0) */
	UWORD  Transparency; /* Transparent background color */
	UBYTE  XAspectRatio; /* Horizontal pixel size */
	UBYTE  YAspectRatio; /* Vertical pixel size */
	UWORD  PageWidth;    /* Horizontal resolution of display device */
	UWORD  PageHeight;   /* Vertical resolution of display device */
};

typedef UWORD (*chunk_callback)(struct IFFstack *stack, struct IFFctx *ctx, ULONG offset) ;

struct IFFstack{
	struct IFFstack *parent;
	UBYTE id;				// auto generated ID
	UBYTE type;
	UBYTE chunk_name[4];
	UBYTE chunk_id[4];
	ULONG remaining;
	ULONG length;
	BOOL align ;
};

struct IFFctx{
	struct Library *gfxlib ;
	FILE *f;
	UBYTE *imageBuffer;
	ULONG size;				// Entire IFF image
};
	
struct IFFgfx{
	struct IFFctx ctx;		// Entire IFF image
	struct _BitmapHeader bitmaphdr;
	ULONG cmap_offset;
	ULONG cmap_length;
	ULONG body_offset;
	ULONG body_length;
	ULONG camg_flags;
};

struct IFFnode{
	struct IFFnode *parent;
	struct IFFnode *child;
	struct IFFnode *previous;
	struct IFFnode *next;
	UBYTE szChunkName[5];
	UBYTE szChunkID[5];
	ULONG length; // actual length (w/o padding), but with chunk identifiers
	FILE *fContent; // NULL if unused, must be open
	UBYTE *bufContent; // NULL if unused, otherwise fContent cannot be set at the same time
	ULONG dataLength; // length of the buffer or file content. Updated for file if set to zero (assumes whole file to be read from offset)
	ULONG dataOffset; // if the data for the node exists at a file or buffer offset then needs setting here
	BOOL literal ; // if the buffer or file contain the literal IFF structure then set to TRUE
};

struct IFFRenderInfo {
	APTR			Memory;
	UWORD			BytesPerRow;
	UWORD			pad;
	UBYTE			RGBFormat;
	UWORD			Width;
	UWORD			Height;
};

UWORD initialiseIFF(struct IFFgfx *iff);
UWORD parseIFFImage(struct IFFgfx *iff, FILE *f);
struct BitMap* createBitMap(struct IFFgfx *gfx,struct BitMap *friend);
void freeBitMap(struct BitMap *bmp);

struct ColorSpec* createColorMap(struct IFFgfx *gfx, UBYTE bitspergun);
void freeColorMap(struct ColorSpec *cs);

//struct BitMap* convertPlanarTo16bit(UWORD Width, UWORD Height, struct BitMap *bmp, struct ColorSpec *cs);
//UWORD convertPlanarTo16bitRender(struct IFFRenderInfo *ri, UWORD Width, UWORD Height, struct BitMap *bmp, struct ColorSpec *cs);
UWORD convertPlanarTo24bitRender(struct IFFRenderInfo *ri, UWORD Width, UWORD Height, struct BitMap *bmp, struct ColorSpec *cs);

// Src or fsrc should be set depending on the data being in a buffer on from file.
// Dest should be preallocated memory which is long enough to unpack an entire image row.
ULONG unpackIFF(UWORD bytesPerRow, UBYTE *dest, UBYTE *src, FILE *fsrc);

// Parse an IFF file. IFFctx can be an extended struct for caller to use in call back function (see struct IFFgfx as an example)
// Parse works up to a depth of IFF_MAX_DEPTH
// Callback function will provide the current chunk name in the stack parameter. Chunk_id provides the type for CAT, FORM, LIST or PROP.
// Parent will point to the containing CAT, FORM, LIST or PROP unless this is the last container. 
// ID is unique for each CAT, FORM, LIST or PROP. This can be used to tell if the container has changed to a new one, suggesting a new entity in IFF. 
UWORD parseIFF(struct IFFctx *iff, chunk_callback fn);

///////////////////////////////////
// Building IFF

// Allocate memory for a new node. The container can be null if this is the root
// container, otherwise the parent node should be specified.
struct IFFnode* createIFFNode(struct IFFnode *container);

// Helper functions to modify IFFnode
struct IFFnode* createIFFCatalogue(struct IFFnode *container, UBYTE *szIdentifier);
struct IFFnode* createIFFList(struct IFFnode *container, UBYTE *szIdentifier);
struct IFFnode* createIFFForm(struct IFFnode *container, UBYTE *szIdentifier);

struct IFFnode* createIFFContainer(struct IFFnode *container, UBYTE *szChunkName, UBYTE *szIdentifier);
#define createIFFCatalogue(cont, ident) createIFFContainer(cont, "CAT ", ident)
#define createIFFList(cont, ident) createIFFContainer(cont, "LIST", ident)
#define createIFFForm(cont, ident) createIFFContainer(cont, "FORM", ident)
#define createIFFProperty(cont, ident) createIFFContainer(cont, "PROP", ident)
#define createIFFChunk(cont, name) createIFFContainer(cont, name, "\0")

// Remove a node - this is the correct way to deallocate a node generated with createIFFNode.
// This will recursively remove all child nodes as well. 
void deleteIFFNode(struct IFFnode *n);

// Provide an open file handle and constructed IFFnode structure.
// This will write an IFF file using the structure of IFFnode.
// Errors returned from function.
UWORD createIFF(FILE *f, struct IFFnode *root);

#endif