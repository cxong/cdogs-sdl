add_subdirectory(hqx/src)
include_directories(hqx/src ${SDL_INCLUDE_DIR})
add_definitions(-DSTATIC)
set(CDOGS_SOURCES
	actor_placement.c
	actors.c
	ai.c
	ai_context.c
	ai_coop.c
	ai_utils.c
	algorithms.c
	AStar.c
	automap.c
	blit.c
	bullet_class.c
	c_array.c
	campaign_entry.c
	campaigns.c
	character.c
	collision.c
	color.c
	config.c
	config_apply.c
	config_json.c
	config_old.c
	damage.c
	defs.c
	draw.c
	draw_buffer.c
	drawtools.c
	events.c
	files.c
	font.c
	game_events.c
	game_loop.c
	game_mode.c
	gamedata.c
	grafx.c
	grafx_bg.c
	handle_game_events.c
	health_pickup.c
	hiscores.c
	hud.c
	input.c
	joystick.c
	json_utils.c
	keyboard.c
	map.c
	map_archive.c
	map_build.c
	map_classic.c
	map_new.c
	map_object.c
	map_static.c
	mission.c
	mission_convert.c
	mouse.c
	music.c
	net_client.c
	net_server.c
	net_util.c
	objective.c
	objs.c
	palette.c
	particle.c
	path_cache.c
	pic.c
	pic_file.c
	pic_manager.c
	pickup.c
	pics.c
	player.c
	player_template.c
	quick_play.c
	screen_shake.c
	sounds.c
	tile.c
	triggers.c
	utils.c
	vector.c
	weapon.c)
set(CDOGS_HEADERS
	actor_placement.h
	actors.h
	ai.h
	ai_context.h
	ai_coop.h
	ai_utils.h
	algorithms.h
	AStar.h
	automap.h
	blit.h
	bullet_class.h
	c_array.h
	campaign_entry.h
	campaigns.h
	character.h
	collision.h
	color.h
	config.h
	config_json.h
	config_old.h
	damage.h
	defs.h
	draw.h
	draw_buffer.h
	drawtools.h
	events.h
	files.h
	font.h
	game_events.h
	game_loop.h
	game_mode.h
	gamedata.h
	grafx.h
	grafx_bg.h
	handle_game_events.h
	health_pickup.h
	hiscores.h
	hud.h
	input.h
	joystick.h
	json_utils.h
	keyboard.h
	map.h
	map_archive.h
	map_build.h
	map_classic.h
	map_new.h
	map_object.h
	map_static.h
	mission.h
	mission_convert.h
	mouse.h
	music.h
	net_client.h
	net_server.h
	net_util.h
	objective.h
	objs.h
	palette.h
	particle.h
	path_cache.h
	pic.h
	pic_file.h
	pic_manager.h
	pickup.h
	pics.h
	player.h
	player_template.h
	quick_play.h
	screen_shake.h
	sounds.h
	sys_config.h
	sys_specifics.h
	tile.h
	triggers.h
	utils.h
	vector.h
	weapon.h)
set(HQX_SOURCES
	hqx/src/common.c
	hqx/src/hq2x.c
	hqx/src/hq3x.c
	hqx/src/hq4x.c
	hqx/src/init.c)
set(HQX_HEADERS
	hqx/src/common.h
	hqx/src/hqx.h)
set(NANOPB_SOURCES
	proto/client.pb.c
	proto/server.pb.c
    proto/nanopb/pb_common.c
    proto/nanopb/pb_decode.c
    proto/nanopb/pb_encode.c
)
set(NANOPB_HEADERS
	proto/client.pb.h
	proto/server.pb.h
    proto/nanopb/pb.h
    proto/nanopb/pb_common.h
    proto/nanopb/pb_decode.h
    proto/nanopb/pb_encode.h
)

SET(PHYSFS_LINK_NAME physfs)
IF(NOT PHYSFS_FOUND)
	# Configure and include PhysFS
	set(PHYSFS_ARCHIVE_7Z FALSE CACHE BOOL "Disable 7Z")
	set(PHYSFS_ARCHIVE_GRP FALSE CACHE BOOL "Disable GRP")
	set(PHYSFS_ARCHIVE_WAD FALSE CACHE BOOL "Disable WAD")
	set(PHYSFS_ARCHIVE_HOG FALSE CACHE BOOL "Disable HOG")
	set(PHYSFS_ARCHIVE_MVL FALSE CACHE BOOL "Disable MVL")
	set(PHYSFS_ARCHIVE_QPAK FALSE CACHE BOOL "Disable QPAK")
	set(PHYSFS_BUILD_SHARED FALSE CACHE BOOL "Disable shared library")
	set(PHYSFS_INTERNAL_ZLIB TRUE CACHE BOOL "Use internal zlib")
	add_subdirectory(physfs)
	SET(PHYSFS_LINK_NAME physfs-static)
	MESSAGE("Using internal physfs library")
ELSE()
	MESSAGE("Using system physfs library")
ENDIF()

# enet
ADD_SUBDIRECTORY(enet)

# proto
include_directories(proto/nanopb)

add_library(cdogs STATIC
	${CDOGS_SOURCES} ${CDOGS_HEADERS}
	${HQX_SOURCES} ${HQX_HEADERS}
	${NANOPB_SOURCES} ${NANOPB_HEADERS})
target_link_libraries(cdogs
	json
	${PHYSFS_LINK_NAME}
	${SDL_LIBRARY}
	${SDLIMAGE_LIBRARY}
	${SDLMIXER_LIBRARY}
	${ENet_LIBRARIES})
