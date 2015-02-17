#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "mcp_packet.h"
#include "mcp_ids.h"

////////////////////////////////////////////////////////////////////////////////

#define GSOP_PRUNE_CHUNKS       1
#define GSOP_SEARCH_SPAWNERS    2
#define GSOP_TRACK_ENTITIES     3

#define ENTITY_UNKNOWN  0
#define ENTITY_SELF     1
#define ENTITY_PLAYER   2
#define ENTITY_MOB      3
#define ENTITY_OBJECT   4
#define ENTITY_OTHER    5

static char * ENTITY_TYPES[] = {
    [ENTITY_UNKNOWN] = "Unknown",
    [ENTITY_SELF]    = "Self",
    [ENTITY_PLAYER]  = "Player",
    [ENTITY_MOB]     = "Mob",
    [ENTITY_OBJECT]  = "Object",
    [ENTITY_OTHER]   = "Other",
};

typedef struct _entity {
    int32_t  id;        // EID
    fixp     x,y,z;     // note: fixed-point coords, divide by 32
    int      type;      // one of the ENTITY_ variables
    int      mtype;     // mob/object type as used natively
    int      hostile;   // whether marked hostile
    uint64_t lasthit;   // timestamp when this entity was last attacked - for limiting the attack rate
    char     name[256]; // only valid for players
} entity;

////////////////////////////////////////////////////////////////////////////////

typedef struct {
    bid_t blocks[65536];
    light_t light[32768];
    light_t skylight[32768];
    uint8_t  biome[256];
} gschunk;

#define CHUNKIDX(w,X,Z) ((X)-(w)->Xo)+((Z)-(w)->Zo)*((w)->Xs)

typedef struct {
    int32_t Xo, Zo;     // chunk coordinate offset
    int32_t Xs, Zs;     // size of the chunk storage
    gschunk **chunks;   // chunk storage

/*
    Assuming X,Z are world coordinates of a chunk
    Xo,Xz would be the world chunk coordinates in the NW corned
    X-Xo, Z-Zo are the offsets of this chunk within database

    If (X-Xo)<0 || (X-Xo)>=Xs , the chunk is not in the database,
    e.g. it needs to be resized if chunk is to be inserted
    The index of the chunk is (X-Xo)+(Z-Zo)*Zs
*/
} gsworld;

////////////////////////////////////////////////////////////////////////////////

typedef struct _gamestate {
    // options
    struct {
        int prune_chunks;
        int search_spawners;
        int track_entities;
    } opt;

    struct {
        uint32_t    eid;
        double      x,y,z;
        uint8_t     onground;
        float       yaw,pitch;
    } own;

    // tracked entities
    lh_arr_declare(entity, entity);

    gsworld         overworld;
    gsworld         end;
    gsworld         nether;
    gsworld        *world;
} gamestate;

extern gamestate gs;

////////////////////////////////////////////////////////////////////////////////

void gs_reset();
void gs_destroy();
int  gs_setopt(int optid, int value);
int  gs_getopt(int optid);

int  gs_packet(MCPacket *pkt);

void dump_entities();
