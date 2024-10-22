//   Copyright 2024 Aidan Holmes
//   IFF file parser

#include "iff.h"
#include <string.h>
#include <proto/exec.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>

#define CMP_HDR(x,y) (x[0] == y[0] && x[1] == y[1] && x[2] == y[2] && x[3] == y[3])

UWORD _parseIFFImage_callback(struct IFFstack *stack, struct IFFctx *ctx, ULONG offset) ;
UWORD initialiseIFF(struct IFFgfx *iff)
{
	memset(iff, 0, sizeof(struct IFFgfx));
	//iff->gfxlib = OpenLibrary("graphics.library",0);
	//if (!iff->gfxlib){
	//	return IFF_NORESOURCE_ERROR;
	//}
	return IFF_NO_ERROR;
}

UWORD _parseIFFImage_callback(struct IFFstack *stack, struct IFFctx *ctx, ULONG offset) 
{
	struct IFFgfx *iff = NULL;
	void *to = NULL;
	ULONG readSize = 0;
	
	iff = (struct IFFgfx*)ctx;
	
	//printf("_parseIFFImage_callback: received %c%c%c%c chunk\n", stack->chunk_name[0],stack->chunk_name[1],stack->chunk_name[2],stack->chunk_name[3]);
	
	if (CMP_HDR(stack->chunk_name, "BMHD")){
		readSize = sizeof(struct _BitmapHeader);
		to = &iff->bitmaphdr;
	}else if(CMP_HDR(stack->chunk_name, "CMAP")){
		iff->cmap_length = stack->length;
		iff->cmap_offset = offset;
	}else if(CMP_HDR(stack->chunk_name, "BODY")){
		iff->body_length = stack->length;
		iff->body_offset = offset;
	}else if(CMP_HDR(stack->chunk_name, "CAMG")){
		readSize = 4;
		to = &iff->camg_flags;
	}
	
	if (to){
		if (stack->length != readSize){
			return IFF_NOTIFF_ERROR;
		}
		if (ctx->f){
			if ((fread(to, 1, readSize, iff->f)) != readSize){
				return IFF_STREAM_ERROR; // Cannot read
			}
		}else{
			memcpy(to, &ctx->imageBuffer[offset], readSize);
		}
	}
	
	return IFF_NO_ERROR ;
}

UWORD parseIFFImage(struct IFFgfx *iff, FILE *f)
{
	iff->f = f;
	return parseIFF((struct IFFctx*)iff, _parseIFFImage_callback);
}

UWORD parseIFF(struct IFFctx *iff, chunk_callback fn)
{
	struct IFFstack stack[IFF_MAX_DEPTH] ;
	UBYTE id = 0;
	UBYTE readBuf[IFF_BUFF_LEN], *buff = NULL, octet = 0, hdr[IFF_ID_LEN+1], i = 0, state = IFF_STATE_NEW_CHUNK, depth=0;
	UWORD remainingBuf = 0, ret =0;
	ULONG fileoffset = 0;
	
	hdr[IFF_ID_LEN] = 0;
	// Check the data inputs
	if (iff->imageBuffer){
		buff = iff->imageBuffer;
		if (iff->size == 0){
			return IFF_NODATA_ERROR;
		}
		remainingBuf = iff->size ;
	}else if(!iff->f){
		return IFF_NODATA_ERROR;
	}
	
	memset(&stack, 0, sizeof(struct IFFstack)*IFF_MAX_DEPTH);

	while(1){
		// Check size condition and exit if done.
		if (remainingBuf == 0){
			if(iff->f){
				// File streaming bit...
				if ((remainingBuf = fread(readBuf, 1, IFF_BUFF_LEN, iff->f)) == 0){
					if (feof(iff->f)){
						break; // no more data in file stream
					}
				}
				iff->size += remainingBuf;
				buff=readBuf;
			}else{
				// Normal buffer
				break; // all done
			}
		}
		//if (state != IFF_STATE_SKIP){
		//	printf("RemainingBuf %u, octet 0x%02X, depth %u, iff->size %u, state %u, depth.length %u, depth.remaining %u\n", remainingBuf, *buff, depth, iff->size, state, stack[depth].length, stack[depth].remaining);
		//}
		remainingBuf--;
		octet = *buff++;
		switch(state){
			case IFF_STATE_NEW_CHUNK:
				hdr[i++] = octet;
				if (i >= IFF_ID_LEN){
					//printf("Found chunk %s\n", hdr);
					memcpy(stack[depth].chunk_name, hdr, 4);
					state = IFF_STATE_CHUNK_LEN; // assume chunk data
					if (CMP_HDR(hdr,"FORM")){
						stack[depth].type = IFF_TYPE_FORM;
						stack[depth].id = id++;
					}else if(CMP_HDR(hdr,"CAT ")){
						stack[depth].type = IFF_TYPE_CAT;
						stack[depth].id = id++;
					}else if(CMP_HDR(hdr,"LIST")){
						stack[depth].type = IFF_TYPE_LIST;
						stack[depth].id = id++;
					}else{ // check if progressed to another group type
						if (depth == 0){
							return IFF_NOTIFF_ERROR;
						}
						
						if(CMP_HDR(hdr,"PROP")){
							stack[depth].type = IFF_TYPE_PROP;
							stack[depth].id = id++;
						}else{						
							state = IFF_STATE_DATA_LEN; // all data items after this point
						}
					}
					i=3; // prep for length
				}
				break;
			case IFF_STATE_CHUNK_LEN:
			case IFF_STATE_DATA_LEN:
				stack[depth].length |= (octet << (8 * i));
				if (i == 0){
					if (stack[depth].length % 2){
						stack[depth].align = TRUE ;
						stack[depth].length += 1;
					}
					// deduct this chunks length from parents
					if (depth > 0){
						for (i = depth; i > 0; i--){
							stack[i-1].remaining -= stack[depth].length;
						}
					}
					stack[depth].remaining = stack[depth].length;
					
					// completed reading the length
					if (state == IFF_STATE_CHUNK_LEN){
						state = IFF_STATE_ID_CHUNK;
					}else{
						state = IFF_STATE_SKIP; // default to skip
						// Callback
						if (fn){
							if (iff->f){
								// remember where the file offset is before passing to callback
								fileoffset = ftell(iff->f);
								fseek(iff->f,iff->size - remainingBuf, SEEK_SET);
							}
							if ((ret=fn(&stack[depth], iff, iff->size - remainingBuf)) != IFF_NO_ERROR){
								if (ret == IFF_ABORT_NO_ERROR){
									ret = IFF_NO_ERROR;
								}
								return ret; // callback abort further parsing
							}
							if (iff->f){
								// Restore the file position to continue parse
								fseek(iff->f,fileoffset, SEEK_SET);
							}
						}
					}
				}else{
					i-- ;
				}
				// i always exits this state as zero
				break;
			case IFF_STATE_SKIP:
				// do nothing
				break;
			case IFF_STATE_ID_CHUNK:
				hdr[i++] = octet;
				if (i >= IFF_ID_LEN){
					//printf("Found chunk ID %s, length %u\n", hdr, stack[depth].length);
					memcpy(stack[depth].chunk_id, hdr, 4);
					if (stack[depth].type == IFF_TYPE_FORM || 
					    stack[depth].type == IFF_TYPE_LIST || 
						stack[depth].type == IFF_TYPE_CAT  ||
						stack[depth].type == IFF_TYPE_PROP){
						// Entering a container type
						++depth ;
						if (depth >= IFF_MAX_DEPTH){
							// too deep, abort
							--depth;
							state = IFF_STATE_SKIP;
						}else{
							// Reset new stack depth
							memset(&stack[depth], 0, sizeof(struct IFFstack));
							stack[depth].parent = &stack[depth-1];
							state = IFF_STATE_NEW_CHUNK;
							i=0;
						}
					}else{
						// Something else - skip
						state = IFF_STATE_SKIP;
					}
						
				}
				break;
			default:
				// invalid state
				return IFF_PARSE_ERROR;
		}
		
		// if a length has been set and not still reading a length then now reading the chunk
		if (stack[depth].length > 0 && state != IFF_STATE_CHUNK_LEN && state != IFF_STATE_DATA_LEN){
			if (stack[depth].remaining == 0){
				if (depth == 0){
					break; // finished top level chunk
				}
				if ((stack[depth].type == IFF_TYPE_FORM || 
				     stack[depth].type == IFF_TYPE_LIST || 
					 stack[depth].type == IFF_TYPE_CAT  ||
					 stack[depth].type == IFF_TYPE_PROP) && depth > 0){
					depth--; // pop back up the stack
				}
				memset(&stack[depth], 0, sizeof(struct IFFstack));
				state = IFF_STATE_NEW_CHUNK;
				i = 0;

			}else{
				--stack[depth].remaining;
			}
		}
	}
	
	return IFF_NO_ERROR;
}

ULONG unpackIFF(UWORD bytesPerRow, UBYTE *dest, UBYTE *src, FILE *fsrc)
{
	// Returns bytes read from source
	// File should be set to next row position on return
	// Decompress file or buffer into iffrow
	
	UBYTE *p = src, c, *q = NULL;
	WORD rlecmd, copy_bytes = 0, repeat_bytes = 0;
	
	q = dest;
	if (!fsrc && !src){
		return 0;
	}
	if (fsrc){
		p = &c;
	}
	while ((q-dest) < bytesPerRow){
		if (fsrc){
			if ((fread(&c, 1, 1, fsrc)) == 0){
				if (feof(fsrc)){
					break; // no more data in file stream
				}
			}
		}else{
			++p;
		}
		
		if (copy_bytes){
			*q++ = *p ;
			--copy_bytes;
		}else if(repeat_bytes){
			memset(q, *p, repeat_bytes);
			q += repeat_bytes;
			repeat_bytes = 0;
		}else{
			rlecmd = (BYTE)*p;
			if (rlecmd >= 0 && rlecmd <= 127){
				// literal copy
				copy_bytes = rlecmd + 1;
			}else if (rlecmd <= -1 && rlecmd >= -127){
				// replicate next byte 
				repeat_bytes = -rlecmd + 1;
			}else{
				// nop
			}
		}
	}
	
	return (ULONG)(p - src);
}

struct ColorSpec* createColorMap(struct IFFgfx *gfx, UBYTE bitspergun)
{
	struct ColorSpec *cs = NULL ;
	UWORD i=0, colors = 0 ;
	UBYTE iffcolorentry[3], *p = NULL, shift = 0;
	
	colors = gfx->cmap_length / 3;
	
	if (gfx->f){			
		// Align file to start of body content
		fseek(gfx->f, gfx->cmap_offset, SEEK_SET);
		cs = (struct ColorSpec *)AllocVec(sizeof(struct ColorSpec) * (colors+1),MEMF_PUBLIC);
		if (!cs){
			return NULL;
		}
	}else{
		p = gfx->imageBuffer + gfx->cmap_offset;
	}
	
	if (bitspergun <= 4){
		shift = 4;
	}
	for(i=0;i<colors;i++){
		if (gfx->f){
			if ((fread(iffcolorentry, 1, 3, gfx->f)) == 0){
				if (feof(gfx->f)){
					break; // no more data in file stream
				}
			}
			p = iffcolorentry;
		}else{
			p = gfx->imageBuffer + gfx->cmap_offset + (i*3);
		}
		cs[i].ColorIndex = i;
		if (bitspergun <= 8){
			cs[i].Red = p[0] >> shift;
			cs[i].Green = p[1] >> shift;
			cs[i].Blue = p[2] >> shift;
		}else{
			cs[i].Red = p[0] << 24;
			cs[i].Green = p[1] << 24;
			cs[i].Blue = p[2] << 24;
		}
	}
	cs[i].ColorIndex = -1; // terminate
	return cs;
}

void freeColorMap(struct ColorSpec *cs)
{
	if (cs){
		FreeVec(cs);
	}
}
/*
struct BitMap* convertPlanarTo16bit(UWORD Width, UWORD Height, struct BitMap *bmp, struct ColorSpec *cs)
{
	UWORD w, h, d;
	UWORD *p = NULL, *mem = NULL ;
	UWORD cidx = 0;
	ULONG roff = 0;
	struct Library *P96Base = NULL ;
	struct BitMap *out = NULL;
	
	if(!(P96Base=OpenLibrary("Picasso96API.library",2))){
		return NULL ;
	}
	
	out = p96AllocBitMap(Width,Height, bmp->Depth, BMF_USERPRIVATE, 0, RGBFB_R5G6B5);
	if (!out){
		goto final;
	}
	mem = (UWORD *)&out->Planes[0];

	
	for (h=0; h < Height; h++){
		roff = h * bmp->BytesPerRow;
		p = mem + (h * out->BytesPerRow) ;
		for (w=0; w < Width; w++){
			cidx = 0;
			for (d=0; d < bmp->Depth; d++){
				if (bmp->Planes[d][roff + (w/8)] & (1 << (7 - (w%8)))){
					cidx |= 1 << d;
				}					
			}
			*p++ = (cs[cidx].Red << 11) | ((cs[cidx].Green & 0x00FC) << 5) | (cs[cidx].Blue & 0x001F);
		}
	}
	
final:
	CloseLibrary(P96Base);
	return out;
}
*/
/*
UWORD convertPlanarTo16bitRender(struct IFFRenderInfo *ri, UWORD Width, UWORD Height, struct BitMap *bmp, struct ColorSpec *cs)
{
	UWORD w, h, d, ret = IFF_NO_ERROR;
	UWORD *p = NULL;
	UWORD cidx = 0;
	ULONG roff = 0;
	struct Library *CyberGfxBase = NULL ;
	
	//if(!(CyberGfxBase=OpenLibrary("cybergraphics.library",41L))){
	//	return IFF_NORTG_ERROR ;
	//}
	
	ri->BytesPerRow = Width * 2 ; // 16bit colour
	ri->RGBFormat = RGBFB_R5G6B5;
	ri->Memory = AllocVec(Height * ri->BytesPerRow, MEMF_PUBLIC);
	if(!ri->Memory){ 
		ret = IFF_NORESOURCE_ERROR;
		goto final;
	}
	
	for (h=0; h < Height; h++){
		roff = h * bmp->BytesPerRow;
		p = (UWORD*)((UBYTE*)ri->Memory + (h * ri->BytesPerRow)) ;
		for (w=0; w < Width; w++){
			cidx = 0;
			for (d=0; d < bmp->Depth; d++){
				if (bmp->Planes[d][roff + (w/8)] & (1 << (7 - (w%8)))){
					cidx |= 1 << d;
				}					
			}
			*p++ = (cs[cidx].Red << 11) | ((cs[cidx].Green & 0x00FC) << 5) | (cs[cidx].Blue & 0x001F);
		}
	}
	
final:
	//CloseLibrary(CyberGfxBase);
	return ret;
}
*/

UWORD convertPlanarTo24bitRender(struct IFFRenderInfo *ri, UWORD Width, UWORD Height, struct BitMap *bmp, struct ColorSpec *cs)
{
	UWORD w, h, d, ret = IFF_NO_ERROR;
	UBYTE *p = NULL;
	UWORD cidx = 0;
	ULONG roff = 0;
	
	ri->BytesPerRow = Width * 3 ; // 24bit colour
	ri->RGBFormat = RECTFMT_RGB;
	ri->Memory = AllocVec(Height * ri->BytesPerRow, MEMF_PUBLIC);
	if(!ri->Memory){ 
		ret = IFF_NORESOURCE_ERROR;
		goto final;
	}
	
	for (h=0; h < Height; h++){
		roff = h * bmp->BytesPerRow;
		p = ((UBYTE*)ri->Memory + (h * ri->BytesPerRow)) ;
		for (w=0; w < Width; w++){
			cidx = 0;
			for (d=0; d < bmp->Depth; d++){
				if (bmp->Planes[d][roff + (w/8)] & (1 << (7 - (w%8)))){
					cidx |= 1 << d;
				}					
			}
			*p++ = cs[cidx].Red;
			*p++ = cs[cidx].Green;
			*p++ = cs[cidx].Blue;
		}
	}
	
final:
	return ret;
}

struct BitMap* createBitMap(struct IFFgfx *gfx, struct BitMap *friend)
{
	// This will read the mask layer but not add to bitmap.
	// TO DO: Mask layer will need routine to extract for use with BitBlt.
	struct BitMap *bmp = NULL ;
	//struct Library *GfxBase = gfx->gfxlib;
	UWORD stride8 = 0, h=0, bytesread = 0, rowwidth = 0;
	UBYTE *p = NULL, *iffrow = NULL, *q=NULL, depthidx=0, maskplanes = 0;
	ULONG srcread = 0;
	
	if (!gfx){
		return NULL;
	}
	
	if (!gfx->f && !gfx->imageBuffer){
		return NULL;
	}
	
	if (!gfx->body_length || !gfx->body_offset){
		// No parsed image
		return NULL;
	}
	
	maskplanes = gfx->bitmaphdr.Masking?1:0;
	
	bmp = AllocBitMap(gfx->bitmaphdr.Width,gfx->bitmaphdr.Height, gfx->bitmaphdr.Bitplanes, /*BMF_DISPLAYABLE | BMF_INTERLEAVED | */BMF_CLEAR, friend);
	
	if (bmp){
		// Work out the length of each row in IFF Body
		//stride8 = (gfx->bitmaphdr.Width/8) + ((gfx->bitmaphdr.Width%16)?2:0);
		stride8 = ((gfx->bitmaphdr.Width+15)/16) *2;
		rowwidth = stride8*(bmp->Depth + maskplanes);
		// Tell me about this bmp
		printf("Created bitmap with bytes per row %u, rows %u, depth %u\n", bmp->BytesPerRow, bmp->Rows, bmp->Depth);
		printf("Stride8 is %u\n", stride8);
		
		// use iffrow buffer for reading or to write decompressed data to
		if (gfx->bitmaphdr.Compress || gfx->f){
			// Allocate memory for a single row of data
			iffrow = AllocVec(rowwidth,MEMF_PUBLIC);
			if (!iffrow){
				freeBitMap(bmp);
				printf("Failed to alloc mem of size %u\n", rowwidth);
				return NULL ;
			}
		}
		if (gfx->f){			
			// Align file to start of body content
			fseek(gfx->f, gfx->body_offset, SEEK_SET);
		}else{
			p = gfx->imageBuffer + gfx->body_offset;
		}
		
		
		// iterate all rows
		for (h=0; h < gfx->bitmaphdr.Height; h++){
			// for each row, set p to point to the start of the data
			if (gfx->bitmaphdr.Compress){
				if (gfx->f){
					unpackIFF(rowwidth, iffrow, NULL, gfx->f);
				}else{
					srcread = unpackIFF(rowwidth, iffrow, p, NULL);
					p += srcread;
				}
			}else{
				// Not compressed data
				if(gfx->f){
					// Read a row of uncompressed data
					if ((fread(iffrow, rowwidth,1, gfx->f)) == 0){
						if (feof(gfx->f)){
							break; // no more data in file stream
						}
					}
				}else{
					// source pointer and iffrow are the same
					iffrow = gfx->imageBuffer + gfx->body_offset + (rowwidth * h);
				}
			}
			
			for (depthidx=0; depthidx < bmp->Depth; depthidx++){
				memcpy(bmp->Planes[depthidx]+(bmp->BytesPerRow * h), &iffrow[stride8*depthidx], stride8);
			}
		}
		if (iffrow){
			FreeVec(iffrow);
		}
	}else{
		printf("AllocBitMap failed\n") ;
	}
	return bmp;
}

void freeBitMap(struct BitMap *bmp)
{
	FreeBitMap(bmp);
}

struct IFFnode* createIFFNode(struct IFFnode *container)
{
	struct IFFnode *n, *i;
	
	n = (struct IFFnode*)AllocVec(sizeof(struct IFFnode), MEMF_PUBLIC | MEMF_CLEAR);
	if (!n){
		return NULL;
	}
	
	if (container){
		if (container->child){
			// Find tail of sibling linked list
			for(i=container->child; i->next != NULL; i = i->next);
			i->next = n;
		}else{
			container->child = n ; // first child in container
		}
		n->parent = container;
	}
	
	//memset(n->szChunkName,'J',4);
	
	return n;
}

void deleteIFFNode(struct IFFnode *n)
{	
	if (!n){ return;}
	
	if (n->child){
		deleteIFFNode(n->child); // recursive delete
	}
	
	// Unlink
	if (n->next){
		n->next->previous = NULL;
	}
	if (n->previous){
		n->previous->next = NULL;
	}

	// stitch sibling list together
	if (n->previous && n->next){
		n->previous->next = n->next; 
		n->next->previous = n->previous;
	}
		
	FreeVec(n);
}

// returns the length including pad bytes
ULONG _calcNodeSize(struct IFFnode *n)
{
	ULONG pos = 0, len =0;
			
	for (; n; n=n->next){
		n->length = 0;
		if (!n->literal){
			n->length = 8; // update length to include chuck name and 4 byte length
		}
		if (n->fContent){
			if (n->dataLength == 0){ // attempt to read length from file
				pos = ftell(n->fContent);
				fseek(n->fContent,0,SEEK_END);
				n->dataLength = ftell(n->fContent) - n->dataOffset;
				n->length += n->dataLength;
				fseek(n->fContent,pos,SEEK_SET); // restore file pointer
			}else{
				n->length += n->dataLength ;
			}
		}else if(n->bufContent){
			n->length += n->dataLength ;
		}else{ // No content		
			if (n->szChunkID[0] != '\0'){
				n->length += 4;
			}
			if (n->child){
				n->dataLength = _calcNodeSize(n->child);
				n->length += n->dataLength;
			}
		}
		len += n->length + (n->length%2); // add the byte padding
	}
	return len;
}

UWORD _copyStreamData(FILE *to, FILE *from, ULONG len)
{
	ULONG amount = 0, whence = 0;
	UBYTE cpyBuf[1024];
	
	whence = ftell(from);
	
	while (len != 0){
		if (len > 1024){
			amount = 1024;
		}else{
			amount = len;
		}
		if (fread(cpyBuf, 1, amount, from) != amount){
			return IFF_STREAM_ERROR;
		}
		if (fwrite(cpyBuf, 1, amount, to) != amount){
			return IFF_STREAM_ERROR;
		}
		len -= amount ;
	}
	// Restore file pointer (allow further copying if required)
	fseek(from,whence,SEEK_SET);
	return IFF_NO_ERROR;
}

UWORD _writeIFF(FILE *f, struct IFFnode *n)
{
	UWORD ret = IFF_NO_ERROR;
	UWORD bytesWritten = 0;
	UBYTE zero = 0;
	
	for (; n; n=n->next){
		bytesWritten = 0;
		if (!n->literal){ // only write chunk identifiers if not literal content
			if (fwrite(n->szChunkName, 1, 4,f) != 4){
				return IFF_STREAM_ERROR;
			}
			if (fwrite(&n->dataLength, 1, 4,f) != 4){
				return IFF_STREAM_ERROR;
			}
			
			bytesWritten = 8;
			
			if (n->szChunkID[0] != '\0'){
				if (fwrite(n->szChunkID, 1, 4,f) != 4){
					return IFF_STREAM_ERROR;
				}
				bytesWritten += 4;
			}
		}
		
		if (n->fContent){
			fseek(n->fContent, n->dataOffset, SEEK_SET);
			_copyStreamData(f, n->fContent, n->length - bytesWritten);
			if (n->length%2){ // write pad byte
				if (fwrite(&zero, 1, 1, f) != 1){
					return IFF_STREAM_ERROR;
				}
			}
		}else if(n->bufContent){
			if(fwrite(n->bufContent+n->dataOffset, 1, n->dataLength, f) != n->dataLength){
				return IFF_STREAM_ERROR;
			}
			if (n->dataLength%2){ // write pad byte
				if (fwrite(&zero, 1, 1, f) != 1){
					return IFF_STREAM_ERROR;
				}
			}
		}else{ // No content in node
			if (n->child){
				if ((ret = _writeIFF(f, n->child)) != IFF_NO_ERROR){
					return ret ; // problem, abort
				}
			}
		}
	}
	return IFF_NO_ERROR;
}

struct IFFnode* createIFFContainer(struct IFFnode *container, UBYTE *szChunkName, UBYTE *szIdentifier)
{
	struct IFFnode *n = NULL;
	
	if (!(n=createIFFNode(container))){
		return NULL;
	}
	strcpy(n->szChunkName, szChunkName);
	strcpy(n->szChunkID, szIdentifier);
	
	
	return n;
}

UWORD createIFF(FILE *f, struct IFFnode *root)
{
	ULONG size = 0;
	
	// Calculate all nodes lengths
	// updates the length member in node
	size = _calcNodeSize(root);
	
	// write to file
	return _writeIFF(f, root);
}