#ifndef VOID_STRUCTS_H
#define VOID_STRUCTS_H 1

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

typedef struct
{
    float distance;   // Distância para trás
    float height;     // Altura acima do jogador
    float yaw;        // Rotação horizontal
    float pitch;      // Rotação vertical
    float smoothness; // Suavidade no movimento
} Camera;

typedef struct 
{
    T3DVec3 position;
    float rotation;
    T3DVec3 scale;
} Transform;


typedef struct
{
	Transform transform;
    rspq_block_t *block;
	T3DModel *model;
    T3DMat4FP *material;
	color_t color;
} Model3d;


typedef struct
{
	T3DVec3 velocity;
	Model3d mesh;
} Pawn;

typedef struct
{
	T3DVec3 velocity;
	Model3d mesh;
	Camera camera;
} Character;


#endif