#include <libdragon.h>
#include <rspq_profile.h>

#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <t3d/t3ddebug.h>

#include "structs.h"
#include "collision.h"
#include "debug.h"
#include "utils.h"

#define X 0
#define Y 1
#define Z 2

static T3DViewport viewport;
static Character player;
static uint8_t colorAmbient[4] = {0xAA, 0xAA, 0xAA, 0xFF};
static T3DAnim animIdle;

DirLight dirLights[4] = {
	{.color = { 0xFF, 0xAA, 0xAA, 0xFF }, .dir = {{  1.0f,  1.0f, 1.0f }}},
	{.color = { 0x00, 0x00, 0x00, 0xFF }, .dir = {{ -1.0f, -1.0f, 0.0f }}},
	{.color = { 0xFF, 0xFF, 0xFF, 0xFF }, .dir = {{  1.0f,  1.0f, 0.0f }}},
	{.color = { 0x00, 0x00, 0x00, 0xFF }, .dir = {{ -1.0f, -1.0f, 1.0f }}}
};

void draw_scene();
void update_light();
void set_look_at();
void setup();
void init_player();
void set_model_transform(Model3d *model, T3DVec3 *position, float rotation, T3DVec3 *scale);
int load_static_model(Model3d *model, const char *filename);



int main()
{

	setup();
	init_player();

	Model3d terrain;
	T3DMat4 tmpMatrix;
	load_static_model(&terrain, "model.t3dm");
	terrain.transform.scale = (T3DVec3){{0.1f, 0.1f, 0.1f}}; //override scale

	terrain.material = (T3DMat4FP *)malloc_uncached(sizeof(T3DMat4FP));
	t3d_mat4_identity(&tmpMatrix);
	t3d_mat4_scale(&tmpMatrix, terrain.transform.scale.v[X], terrain.transform.scale.v[Y], terrain.transform.scale.v[Z]);
	t3d_mat4_to_fixed(terrain.material, &tmpMatrix);

	float playerRot = 0.0f;
	float lightCountTimer = 0.5f;
	float lastTime = get_time_s() - (1.0f / 60.0f);

	t3d_anim_set_playing(&animIdle, true);
	t3d_anim_set_time(&animIdle, 1.0f);

	while(1){
		float newTime = get_time_s();
		float deltaTime = newTime - lastTime;
		lastTime = newTime;

		debugf("%f\n", deltaTime);

		// ======== Update ======== //
		joypad_poll();
		joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
		joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);

		if (joypad.stick_x < 10 && joypad.stick_x > -10)
			joypad.stick_x = 0;
		if (joypad.stick_y < 10 && joypad.stick_y > -10)
			joypad.stick_y = 0;

		playerRot += 0.05f;

		// Player movement
		// player.mesh.transform.rotation += joypad.stick_x * 0.0007f;
		// T3DVec3 moveDir = {{fm_cosf(player.mesh.transform.rotation) * (joypad.stick_y * 0.006f), 0.0f,
		// 					fm_sinf(player.mesh.transform.rotation) * (joypad.stick_y * 0.006f)}};

		// set_model_transform(&player.mesh, &moveDir, playerRot, NULL);

		// ======== Draw (3D) ======== //
		draw_scene();
		update_light();

		// draw player-models
		t3d_matrix_set(player.mesh.material, true);
		rdpq_set_prim_color(player.mesh.color);
		t3d_matrix_set(player.mesh.material, true);
		rspq_block_run(player.mesh.block);

		rdpq_set_prim_color(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
		t3d_matrix_set(terrain.material, true);
		rspq_block_run(terrain.block);

		
		// t3d_anim_update(&animIdle, deltaTime * 1000);


		// We now blend the walk animation with the idle/attack one
		t3d_skeleton_blend(&player.skeleton, &player.skeleton, &player.skeletonBlend, 1.0f);


		// Now recalc. the matrices, this will cause any model referencing them to use the new pose
		t3d_skeleton_update(&player.skeleton);

		t3d_mat4fp_from_srt_euler(player.mesh.material,
								  (float[3]){0.256f, 0.256f, 0.256f},
								  (float[3]){0.0f, 1.0f, 0},
								  (float[3]){-50, 0, 50});


		// Render on screen
		rdpq_detach_show();
		rspq_profile_next_frame();

		// if(frame == 30)
		// {
		// 	frame = 0;
		// 	rspq_profile_reset();
		// }
	}
}

void update_light(){
	t3d_light_set_count(0);

	t3d_vec3_norm(&dirLights[0].dir);

	// if you need directional light, re-apply it here after a new viewport has been attached
	t3d_light_set_directional(0, &dirLights[0].color.r, &dirLights[0].dir);

}

void draw_scene(){
	rdpq_attach(display_get(), display_get_zbuf());

	t3d_frame_start();
	rdpq_mode_fog(RDPQ_FOG_STANDARD);
	rdpq_set_fog_color(RGBA32(160, 110, 200, 0xFF));

	t3d_light_set_ambient(colorAmbient);

	t3d_screen_clear_color(RGBA32(160, 110, 200, 0xFF));
	t3d_screen_clear_depth();

	t3d_fog_set_range(12.0f, 85.0f);

	t3d_matrix_pop(1);

	set_look_at();

	t3d_viewport_attach(&viewport);

	t3d_matrix_push_pos(1);

}

void setup()
{
	console_init();
	debug_init_isviewer();
	debug_init_usblog();
	asset_init_compression(2);
	rspq_profile_start();

	dfs_init(DFS_DEFAULT_LOCATION);

	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS);

	rdpq_init();

	joypad_init();
	t3d_init((T3DInitParams){});

	int sizeX = display_get_width();
	int sizeY = display_get_height();

	// Here we allocate multiple viewports to render to different parts of the screen
	// This isn't really any different to other examples, just that we have 3 of them now
	viewport = t3d_viewport_create();
	t3d_viewport_set_area(&viewport, 0, 0, sizeX, sizeY);

}

int load_static_model(Model3d *model, const char *filename){
	if (!model || !filename)
	{
		printf("Erro: Parâmetros inválidos para render_model3d\n");
		return -1;
	}

	char filepath[32]; // Buffer para o caminho completo
	snprintf(filepath, sizeof(filepath), "rom:/%s", filename);

	// Carrega o modelo 3D
	T3DModel *loaded_model = t3d_model_load(filepath);
	if (!loaded_model)
	{
		printf("Erro ao carregar o modelo: %s\n", filepath);
		return -1;
	}

	// Cria o bloco de comandos RSP para renderização
	rspq_block_t *dplMap;
	rspq_block_begin();
	t3d_model_draw(loaded_model);
	dplMap = rspq_block_end();

	// Armazena as referências no struct
	model->model = loaded_model;
	model->block = dplMap;

	printf("Modelo carregado e bloco configurado: %s\n", filepath);
	return 0;
}

void init_player(){
	player.mesh.model = t3d_model_load("rom:/snake.t3dm");

	// First instantiate skeletons, they will be used to draw models in a specific pose
	// And serve as the target for animations to modify
	player.skeleton = t3d_skeleton_create(player.mesh.model);
	player.skeletonBlend = t3d_skeleton_clone(&player.skeleton, false); // optimized for blending, has no matrices

	// Now create animation instances (by name), the data in 'model' is fixed,
	// whereas 'anim' contains all the runtime data.
	// Note that tiny3d internally keeps no track of animations, it's up to the user to manage and play them.
	animIdle = t3d_anim_create(player.mesh.model, "Snake_Idle");
	t3d_anim_attach(&animIdle, &player.skeleton); // tells the animation which skeleton to modify



    player.mesh.transform.position = (T3DVec3){{-50, 0, 50}};
    player.mesh.transform.scale = (T3DVec3){{10, 10, 10}};
	player.mesh.transform.rotation = 0;
	player.mesh.color = RGBA32(220, 100, 100, 0xFF),

	player.velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};

	player.camera.distance = 30.0f;
	player.camera.height = 15.0f;


	// Alocar memória para a matriz de transformação
    player.mesh.material = (T3DMat4FP *)malloc_uncached(sizeof(T3DMat4FP));
    if (player.mesh.material == NULL) {
        // Lidar com erro de memória
        printf("Erro: Falha ao alocar memória para transform.\n");
        exit(1);
	}

	rspq_block_begin();
	t3d_matrix_push(player.mesh.material);
	rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
	t3d_model_draw_skinned(player.mesh.model, &player.skeleton); // as in the last example, draw skinned with the main skeleton
	t3d_matrix_pop(1);
	player.mesh.block = rspq_block_end();

}

void set_model_transform(Model3d *model, T3DVec3 *position, float rotation, T3DVec3 *scale){
	if (position != NULL) {
		t3d_vec3_add(&player.mesh.transform.position, &player.mesh.transform.position, position);
		check_map_collision(&player.mesh.transform.position);
	}

	if (rotation != 0){
		t3d_mat4fp_from_srt_euler(player.mesh.material,
			(float[3]){0.06f, 0.06f + fm_sinf(rotation) * 0.005f, 0.06f},
			(float[3]){0.0f, rotation, 0.0f},
			player.mesh.transform.position.v);
	}

	if(scale != NULL) {

	}
}

void set_look_at(){

	// Calcular a posição da câmera atrás do jogador
	T3DVec3 camPos = {{
		player.mesh.transform.position.v[0] - player.camera.distance * fm_cosf(player.mesh.transform.rotation), // Atrás do jogador
		player.mesh.transform.position.v[1] + player.camera.height,						  						// Acima do jogador
		player.mesh.transform.position.v[2] - player.camera.distance * fm_sinf(player.mesh.transform.rotation)  // Atrás do jogador
	}};

	// O ponto que a câmera está "olhando" (normalmente o jogador)
	T3DVec3 camTarget = {{
		player.mesh.transform.position.v[0],		  	// Centro do jogador
		player.mesh.transform.position.v[1] + 10.0f, 	// Um pouco acima do centro para evitar a base
		player.mesh.transform.position.v[2]		  		// Centro do jogador
	}};

	t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(75.0f), 2.0f, 200.0f);
	t3d_viewport_look_at(&viewport, &camPos, &camTarget, &(T3DVec3){{0, 1, 0}});

}
