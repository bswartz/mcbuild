#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LH_DECLARE_SHORT_NAMES 1

#include <lh_debug.h>
#include <lh_arr.h>
#include <lh_bytes.h>
#include <lh_files.h>
#include <lh_compress.h>

#include "mcp_gamestate.h"
#include "mcp_ids.h"
#include "mcp_packet.h"

#define STATE_IDLE     0
#define STATE_STATUS   1
#define STATE_LOGIN    2
#define STATE_PLAY     3

static char * states = "ISLP";

void parse_mcp(uint8_t *data, ssize_t size) {
    uint8_t *hdr = data;
    int state = STATE_IDLE;

    int compression = 0; // compression disabled initially
    BUFI(udata);         // buffer for decompressed data

    int max=20;
    while(hdr-data < (size-16) && max>0) {
        //max--;
        uint8_t *p = hdr;

        Rint(is_client);
        Rint(sec);
        Rint(usec);
        Rint(len);

        //printf("%d.%06d: len=%d\n",sec,usec,len);
        uint8_t *lim = p+len;
        if (lim > data+size) {printf("incomplete packet\n"); break;}

        if (compression) {
            // compression was enabled previously - we need to handle packets differently now
            Rvarint(usize); // size of the uncompressed data
            if (usize > 0) {
                // this packet is compressed, unpack and move the decoding pointer to the decoded buffer
                arr_resize(GAR(udata), usize);
                ssize_t usize_ret = zlib_decode_to(p, data+size-p, AR(udata));
                p = P(udata);
                lim = p+usize;
            }
            // usize==0 means the packet is not compressed, so in effect we simply moved the
            // decoding pointer to the start of the actual packet data
        }

        ssize_t plen=lim-p;
        //printf("%d.%06d: %c %c type=?? plen=%6zd    ",sec,usec,is_client?'C':'S',states[state],plen);
        //hexprint(p, (plen>64)?64:plen);

        if (state == STATE_PLAY) {
            MCPacket *pkt = decode_packet(is_client, p, plen);
            dump_packet(pkt);
            free_packet(pkt);
        }

        uint8_t *ptr = p;
        // pkt will point at the start of packet (specifically at the type field)
        // while p will move to the next field to be parsed.

        Rvarint(type);
        //printf("%d.%06d: %c type=%x len=%zd\n",sec,usec,is_client?'C':'S',type,len);

        uint32_t stype = ((state<<24)|(is_client<<28)|(type&0xffffff));

        
        //if (state == STATE_PLAY)
        //    import_packet(pkt, len, is_client);

        switch (stype) {
            case CI_Handshake: {
                Rvarint(protocolVer);
                Rstr(serverAddr);
                Rshort(serverPort);
                Rvarint(nextState);
                state = nextState;
                break;
            }
            case SL_LoginSuccess: {
                state = STATE_PLAY;
                break;
            }

            case SP_SetCompression:
            case SL_SetCompression: {
                compression = 1;
                break;
            }
        }
    
        hdr += 16+len; // advance header pointer to the next packet
    }

    lh_free(P(udata));
}

////////////////////////////////////////////////////////////////////////////////

void search_blocks(uint8_t id) {
    int i,j;
    switch_dimension(DIM_END);
    printf("Current: %zd, Overworld: %zd, Nether: %zd, End: %zd\n",
           gs.C(chunk),gs.C(chunko),gs.C(chunkn),gs.C(chunke));

    printf("searching through %zd chunks\n",gs.C(chunko));

    for(i=0; i<gs.C(chunko); i++) {
        int X = P(gs.chunko)[i].X;
        int Z = P(gs.chunko)[i].Z;
        chunk *c = P(gs.chunko)[i].c;
        for(j=0; j<65536; j++) {
            if (c->blocks[j] == id) {
                printf("%d,%d\n",X,Z);
                break;
            }
        }
    }
}

int main(int ac, char **av) {
    reset_gamestate();
    set_option(GSOP_PRUNE_CHUNKS, 0);
    set_option(GSOP_SEARCH_SPAWNERS, 1);

    if (!av[1]) LH_ERROR(-1, "Usage: %s <file.mcs>", av[0]);

    int i;
    for(i=1; av[i]; i++) {
        uint8_t *data;
        ssize_t size = lh_load_alloc(av[i], &data);
        if (size < 0) {
            fprintf(stderr,"Failed to open %s : %s",av[i],strerror(errno));
        }
        else {
            parse_mcp(data, size);
            free(data);
        }
    }
    //search_spawners();
    //search_blocks(0xa2);
}
