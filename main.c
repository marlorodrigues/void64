#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

#include "structs.h"
#include "collision.h"

#define X 0
#define Y 1
#define Z 2

static T3DViewport viewport;
static Character player;
static uint8_t colorAmbient[4] = {250, 220, 220, 0xFF};

void draw_scene();

void setup()
{
	debug_init_isviewer();
	debug_init_usblog();
	asset_init_compression(2);

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
	player.mesh.model = t3d_model_load("rom:/cube.t3dm");
    player.mesh.transform.position = (T3DVec3){{-50, 0, 50}};
    player.mesh.transform.scale = (T3DVec3){{1, 1, 1}};
	player.mesh.transform.rotation = 0;
	player.mesh.color = RGBA32(220, 100, 100, 0xFF),

	player.velocity = (T3DVec3){{0.0f, 0.0f, 0.0f}};

	player.camera.distance = 10.0f;
	player.camera.height = 10.0f;

	
	// Alocar memória para a matriz de transformação
    player.mesh.material = (T3DMat4FP *)malloc_uncached(sizeof(T3DMat4FP));
    if (player.mesh.material == NULL) {
        // Lidar com erro de memória
        printf("Erro: Falha ao alocar memória para transform.\n");
        exit(1);
	}

	rspq_block_begin();
	t3d_model_draw(player.mesh.model);
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
		player.mesh.transform.position.v[1] + player.camera.height,						  // Acima do jogador
		player.mesh.transform.position.v[2] - player.camera.distance * fm_sinf(player.mesh.transform.rotation)  // Atrás do jogador
	}};

	// O ponto que a câmera está "olhando" (normalmente o jogador)
	T3DVec3 camTarget = {{
		player.mesh.transform.position.v[0],		  // Centro do jogador
		player.mesh.transform.position.v[1] + 10.0f, // Um pouco acima do centro para evitar a base
		player.mesh.transform.position.v[2]		  // Centro do jogador
	}};

	t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(75.0f), 2.0f, 200.0f);
	t3d_viewport_look_at(&viewport, &camPos, &camTarget, &(T3DVec3){{0, 1, 0}});

}

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
	for (;;)
	{
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
		player.mesh.transform.rotation += joypad.stick_x * 0.0007f;
		T3DVec3 moveDir = {{fm_cosf(player.mesh.transform.rotation) * (joypad.stick_y * 0.006f), 0.0f,
							fm_sinf(player.mesh.transform.rotation) * (joypad.stick_y * 0.006f)}};

		set_model_transform(&player.mesh, &moveDir, playerRot, NULL);

		// ======== Draw (3D) ======== //
		draw_scene();
		
		// draw player-models
		t3d_matrix_set(player.mesh.material, true);
		rdpq_set_prim_color(player.mesh.color);
		t3d_matrix_set(player.mesh.material, true);
		rspq_block_run(player.mesh.block);

		rdpq_set_prim_color(RGBA32(0xFF, 0xFF, 0xFF, 0xFF));
		t3d_matrix_set(terrain.material, true);
		rspq_block_run(terrain.block);

		// Render on screen
		rdpq_detach_show();
	}
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

	// if you need directional light, re-apply it here after a new viewport has been attached
	// t3d_light_set_directional(0, colorDir, &lightDirVec);
	t3d_matrix_push_pos(1);

}

