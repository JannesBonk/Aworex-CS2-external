#include "../../../inc.hpp"
#include "esp.hpp"
#include "../../../sdk/animation_system/animation_system.hpp"

void awo::esp_t::setup_alpha( const c_entity& local_player, const c_entity& entity, int local_index, int index ) { 
	float frame_time = 0.015;
	float delta_time = ( 1.0f / 0.3f ) * frame_time * 0.10f;

	bool alive_player = entity.player_pawn.health > 0; /* health > 0 */
	/* grab distance from local to player */
	float distance = local_player.player_pawn.vec_origin.dist_to( entity.player_pawn.vec_origin );

	/* allowed distance */
	static const float run_dormancy = 750.f;

	if ( alive_player ) { /* && !( ( distance > run_dormancy ) && !this->spotted( entity, local_player, local_index, index )*/
		this->esp_alpha[ index ] += 255.f / 0.68f * frame_time;
		this->esp_alpha[ index ] = std::clamp( this->esp_alpha[ index ], 0.f, 255.f );
	} else if ( !alive_player ) {
		this->esp_alpha[ index ] -= 255.f / 0.68f * frame_time;
		this->esp_alpha[ index ] = std::clamp( this->esp_alpha[ index ], 0.f, 255.f );
	}
	//else if ( alive_player && ( distance > run_dormancy ) && awo::_settings->dormancy ) {
	//	if ( esp_alpha[ index ] < 150.f ) {
	//		this->esp_alpha[ index ] -= 255.f / 16.f * frame_time;
	//		this->esp_alpha[ index ] = std::clamp( this->esp_alpha[ index ], 0.f, 160.f );
	//	} else {
	//		this->esp_alpha[ index ] -= 255.f / 1.f * frame_time;
	//	}
	//}
}

void awo::esp_t::bounding_box( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) {
	awo::col_t box_color{ };
	if ( awo::_settings->change_by_visibility ) {
		box_color = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->box_color ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->box_color_inv ).modify_alpha( this->esp_alpha[ index ] );
	}  else {
		box_color = _render->to_main_color( _settings->box_color ).modify_alpha( this->esp_alpha[ index ] );
	}

	awo::_render->add_rect(
		rect.x, rect.y, /* bb.min */
		rect.z, rect.w, /* bb.max */
		box_color, 0, 1.f
	);

	//
	//awo::_render->add_rect(
	//	rect.x - 1, rect.y - 1, /* bb.min */
	//	rect.z + 2, rect.w + 2, /* bb.max */
	//	awo::col_t( 0, 0, 0 ).modify_alpha( this->esp_alpha[ index ] ), 0, 1.f
	//);


	//awo::_render->add_rect(
	//	rect.x + 1, rect.y + 1, /* bb.min */
	//	rect.z - 2, rect.w - 2, /* bb.max */
	//	awo::col_t( 0, 0, 0 ).modify_alpha( this->esp_alpha[ index ] ), 0, 1.f
	//);
}

void awo::esp_t::render_name( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { 
	awo::col_t name_color{ };
	if ( awo::_settings->change_by_visibility ) {
		name_color = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->name_color ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->name_color_inv ).modify_alpha( this->esp_alpha[ index ] );
	} else {
		name_color = _render->to_main_color( _settings->name_color ).modify_alpha( this->esp_alpha[ index ] );
	}

	/* player name */
	std::string player_name = entity.controller.player_name;

	// apache - sanity fix for the retards who want to crash our game ;)
	if (player_name.length() > 36)
	{
		player_name.erase(36, player_name.length() - 36);
		player_name.append(("..."));
	}

	// prnate - force lowercase for the aesthetics yk
	std::for_each(player_name.begin(), player_name.end(), [](char& c) {
		c = ::tolower(c);
		});

	/* text sizes */
	int text_width = awo::_render->font_tahoma->CalcTextSizeA(15.f, FLT_MAX, NULL, player_name.c_str()).x;
	int text_height = awo::_render->font_tahoma->CalcTextSizeA(15.f, FLT_MAX, NULL, player_name.c_str()).y;
	
	
	awo::_render->add_text(
		rect.x + ( rect.z  * 0.5f ) - ( text_width * 0.5f ), rect.y - text_height - 2, name_color, awo::_render->font_tahoma, 
		player_name.c_str( ), awo::font_flags_t::outline
	);

	if (awo::_settings->name_animation) {
		awo::_render->bind_animation(index, player_name.c_str(), _render->to_main_color(_settings->name_color_a).modify_alpha(this->esp_alpha[index]),
			awo::_render->font_tahoma, rect.x + (rect.z * 0.5f) - (text_width * 0.5f), rect.y - text_height - 2, (awo::text_animation_t)awo::_settings->name_at, 0.1);
	}

}

void awo::esp_t::health_bar( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { 
	int player_hp = entity.player_pawn.health;
	int max_hp = 100;

	static float prev_player_hp[ 64 ];
	constexpr float speed_freq = 255 / 1.0f;

	if ( prev_player_hp[ index ] > player_hp )
		prev_player_hp[ index ] -= speed_freq * 0.015;
	else
		prev_player_hp[ index ] = player_hp;

	/* calc hp color */
	int r = std::fmin( ( 510 * ( 100 - player_hp ) ) / 100, 255 );
	int g = std::fmin( ( 510 * player_hp ) / 100, 255 );

	int h = rect.w + 2;
	int fill = ( int )std::round( prev_player_hp[ index ] * h / 100.f );

	/* health color */
	awo::col_t health_color{ };
	if ( awo::_settings->change_by_visibility && awo::_settings->customhealthbar ) {
		health_color = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->healthbar ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->healthbari ).modify_alpha( this->esp_alpha[ index ] );
	} else if ( awo::_settings->customhealthbar ) { 
		health_color = _render->to_main_color( _settings->healthbar ).modify_alpha( this->esp_alpha[ index ] );
	} else {
		health_color = awo::col_t( r, g, 0 ).modify_alpha( this->esp_alpha[ index ] );
	}

	/* backround */
	awo::_render->add_rect_filled( rect.x - 6, rect.y - 1, 4, rect.w + 2, awo::col_t( 0, 0, 0, this->esp_alpha[ index ] * 0.5 ), 0 );

	switch ( _settings->visuals_i[0] ) {
		case 0:
		{
			awo::_render->add_rect_filled( rect.x - 5, rect.y + ( h - fill ), 2, fill - 2, health_color, 0 );
		} break;
		case 1:
		{
			awo::_render->add_gradient_horizontal( rect.x - 5, rect.y + ( h - fill ), 2, fill - 2, health_color, _render->to_main_color( _settings->visuals_c[ 0 ] ).modify_alpha( this->esp_alpha[ index ] ) );
		} break;
	}

	if ( player_hp < 98 ) {
		awo::_render->add_text( rect.x - ( ( player_hp > 10 ) ? 8 : 7 ),
								rect.y + 1 + ( ( h - fill ) - awo::_render->font_small_pixel->FontSize), awo::col_t().modify_alpha( this->esp_alpha[ index ] ), awo::_render->font_small_pixel, std::to_string( player_hp ).c_str(), awo::font_flags_t::outline);
	}
}

void awo::esp_t::ammo_bar( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { 
	if ( entity.player_pawn.weapon_type == awo::cs_weapon_type::weapon_type_knife || entity.player_pawn.weapon_type == awo::cs_weapon_type::weapon_type_c4 ||
		 entity.player_pawn.weapon_type == awo::cs_weapon_type::weapon_type_grenade ) {
		return; /* do not run */
	}

	int bullets = entity.player_pawn.clip;
	int max = entity.player_pawn.max_clip;

	/* we gonna init them later */
	int bar;
	float scale;

	awo::col_t ammo_color{ };
	if ( awo::_settings->change_by_visibility ) {
		ammo_color = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->ammobar_color ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->ammobar_color_inv ).modify_alpha( this->esp_alpha[ index ] );
	} else {
		ammo_color = _render->to_main_color( _settings->ammobar_color ).modify_alpha( this->esp_alpha[ index ] );
	}

	if ( max != -1 ) {
		scale = ( float )bullets / max;
		bar = ( int )std::round( ( rect.z + 2 ) * scale );

		awo::_render->add_rect_filled( rect.x - 1, rect.y + rect.w + 2, rect.z + 2, 4, awo::col_t( 0, 0, 0, this->esp_alpha[ index ] * 0.5 ), 0 );

		switch ( _settings->visuals_i[ 1 ] ) {
			case 0:
			{
				awo::_render->add_rect_filled( rect.x, rect.y + rect.w + 3, bar - 2, 2, ammo_color, 0 );
			} break;
			case 1:
			{
				awo::_render->add_gradient_vertical( rect.x, rect.y + rect.w + 3, bar - 2, 2, ammo_color, _render->to_main_color( _settings->visuals_c[ 1 ] ).modify_alpha( this->esp_alpha[ index ] ) );
			} break;
		}

		if ( bullets < ( max - 2 ) ) {


			std::for_each(std::to_string(bullets).begin(), std::to_string(bullets).end(), [](char& c) {
				c = ::tolower(c);
				});

			awo::_render->add_text( rect.x - 1 + bar,
									rect.y - 3 + rect.w + 2, awo::col_t( ).modify_alpha( this->esp_alpha[ index ] ), awo::_render->font_small_pixel, std::to_string( bullets ).c_str( ), awo::font_flags_t::outline );
		}
	}
}

void awo::esp_t::render_weapon( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { 
	std::string weaepon_name = entity.player_pawn.weapon_name;

	/* we re gonna initialize this later */
	int offset = awo::_settings->ammobar ? 5 : 0;

	/* transform it */
	std::transform( weaepon_name.begin( ), weaepon_name.end( ), weaepon_name.begin( ), ::toupper );

	awo::col_t weapon_clr{ };
	if ( awo::_settings->change_by_visibility ) {
		weapon_clr = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->eap_color ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->eap_color_inv ).modify_alpha( this->esp_alpha[ index ] );
	} else {
		weapon_clr = _render->to_main_color( _settings->eap_color ).modify_alpha( this->esp_alpha[ index ] );
	}

	/* text sizes */
	int text_width = awo::_render->text_size( weaepon_name.c_str( ), awo::_render->font_tahoma).x;
	int text_height = awo::_render->text_size( weaepon_name.c_str( ), awo::_render->font_tahoma).y;

	std::for_each(weaepon_name.begin(), weaepon_name.end(), [](char& c) {
		c = ::tolower(c);
		});

	awo::_render->add_text( rect.x + ( rect.z * 0.5f ) - ( text_width * 0.5f ), rect.y + rect.w + 1 + offset + ( _settings->visuals_b[ 0 ] ? text_height : 0 ), weapon_clr,
		awo::_render->font_tahoma, weaepon_name.c_str( ), awo::font_flags_t::outline);

	if (awo::_settings->weapon_animation) {
		awo::_render->bind_animation(index, weaepon_name.c_str(), _render->to_main_color(_settings->weapon_color_a).modify_alpha(this->esp_alpha[index]),
			awo::_render->font_tahoma, rect.x + (rect.z * 0.5f) - (text_width * 0.5f), rect.y + rect.w + 1 + offset + (_settings->visuals_b[0] ? text_height : 0), (awo::text_animation_t)awo::_settings->weapon_at, 0.1);
	}

}

void awo::esp_t::skeleton_esp( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) {
	awo::bone_pos previous, current;

	awo::col_t skele_clr{ };
	if ( awo::_settings->change_by_visibility ) {
		skele_clr = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->bone_color ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->bone_color_inv ).modify_alpha( this->esp_alpha[ index ] );
	} else {
		skele_clr = _render->to_main_color( _settings->bone_color ).modify_alpha( this->esp_alpha[ index ] );
	}

	if ( awo::_settings->bones ) {
		/* iterate through bones */
		for ( auto i : bone_list::hitbox_list ) {
			previous.pos = vec3_t( 0, 0, 0 );
			for ( auto index : i ) {
				current = entity.get_bone( ).bone_pos_list[ index ];
				if ( previous.pos == vec3_t( 0, 0, 0 ) ) {
					previous = current;
					continue;
				}
				if ( previous.is_visible && current.is_visible ) {
					awo::_render->add_line( previous.screen_pos, current.screen_pos, skele_clr, 1.0f );
				}
				previous = current;
			}
		}
	}

	/* head dot */
	if ( awo::_settings->bones_h ) {
		bone_pos head = entity.get_bone( ).bone_pos_list[ bone_index::head ];
		awo::_render->add_circle( head.screen_pos, 6, skele_clr );
	}
}

void awo::esp_t::render_side_info( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) {
	struct info_t {
		std::string inf;
		awo::col_t col;
	};
	
	std::vector<info_t> info{};

	if ( ( entity.player_pawn.health < 30 ) ) {
		info.push_back( { "lethal", awo::col_t( 255, 71, 71 ) } );
	}

#if 0
	if ( awo::_settings->show_competivie_wins ) {
		info.push_back( { "cw: " + std::to_string( entity.controller.wins ), awo::col_t( ) } ); // i guess it works
	}
#endif

	if ( awo::_settings->show_dmg_dealt ) {
		info.push_back( { "td: " + std::to_string( entity.controller.total_dmg ), awo::col_t( ) } ); // i guess it works
	}

	/* beta */
	info.push_back( { std::to_string( entity.controller.money ) + "$", awo::col_t( 66, 245, 84 ) } ); // i guess it works
	info.push_back( { std::to_string( entity.controller.ping ) + "ms", awo::col_t( ) } ); // i guess it works

	for ( int i = 0; i < info.size( ); i++ ) {
		/* transform it */
		std::transform( info[i].inf.begin( ), info[ i ].inf.end( ), info[ i ].inf.begin( ), ::tolower );

		awo::_render->add_text( rect.x + rect.z + 6, rect.y + 1 + 
								( ( awo::_render->text_size( info[ i ].inf.c_str( ), awo::_render->font_small_pixel).y + 1 ) ) * i, info[ i ].col.modify_alpha( this->esp_alpha[ index ] ), awo::_render->font_small_pixel, info[ i ].inf.c_str( ), awo::font_flags_t::outline );
	}
}

void awo::esp_t::render_dropped_esp( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { /* shit doesnt work */
	DWORD64 game_scene_node = 0;

	if ( !_proc_manager.read_memory<DWORD64>( entity.player_pawn.address + offsets::c_base_entity::game_scene_node, game_scene_node ) ) {
#ifdef read_data_dbg
		print_with_data_scoped( "ccs_player_pawn::_dormant -> error -> no memory" );
#endif // read_data_dbg
		return;
	}

	for ( int i = 65; i < 1024; i++ ) {
		uintptr_t entity_uint;// = _proc_manager.read_memory<uintptr_t>( entity.player_pawn.address + 0x10 );
		if ( !_proc_manager.read_memory<uintptr_t>( entity.player_pawn.address + 0x10, entity_uint ) ) {

			printf( "xx1" );
			continue;
		}


		uintptr_t designerNameAddy;
		if ( !_proc_manager.read_memory<uintptr_t>( entity_uint + 0x20, designerNameAddy ) ) {
			printf( "xx12" );
			continue;
		}

		char designerNameBuffer[ MAX_PATH ]{};
		_proc_manager.read_memory( designerNameAddy, designerNameBuffer, MAX_PATH );

		std::string name = std::string( designerNameBuffer );

		if ( strstr( name.c_str( ), "weapon_" ) ) name.erase( 0, 7 );
		else if ( strstr( name.c_str( ), "_projectile" ) ) name.erase( name.length( ) - 11, 11 );
		else if ( strstr( name.c_str( ), "baseanimgraph" ) ) name = "defuse kit";
		else continue;

		name = entity.player_pawn.weapon_name;

		std::for_each(name.begin(), name.end(), [](char& c) {
			c = ::tolower(c);
			});

		awo::_render->add_text( entity.player_pawn.vec_origin.x, entity.player_pawn.vec_origin.y, awo::col_t( ), awo::_render->font_tahoma, name.c_str( ) );
	}
}

void awo::esp_t::render_distance( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) {
	if ( !_settings->visuals_b[ 0 ] )
		return;

    /* we re gonna initialize this later */
	int offset = awo::_settings->ammobar ? 5 : 0;
	int distance = local_player.player_pawn.vec_origin.dist_to( entity.player_pawn.vec_origin ) ; // * 0.01904f

	std::string dist = std::to_string( distance ) + "FT";

	awo::col_t clr{ };
	if ( awo::_settings->change_by_visibility ) {
		clr = this->spotted( entity, local_player, local_index, index ) ? _render->to_main_color( _settings->visuals_c[2] ).modify_alpha( this->esp_alpha[ index ] ) : _render->to_main_color( _settings->visuals_c[ 2 ] ).modify_alpha( this->esp_alpha[ index ] );
	} else {
		clr = _render->to_main_color( _settings->visuals_c[ 2 ] ).modify_alpha( this->esp_alpha[ index ] );
	}

	/* text sizes */
	int text_width = awo::_render->text_size( dist.c_str( ), awo::_render->font_tahoma).x;
	int text_height = awo::_render->text_size( dist.c_str( ), awo::_render->font_tahoma).y;

	std::for_each(dist.begin(), dist.end(), [](char& c) {
		c = ::tolower(c);
		});

	awo::_render->add_text( rect.x + ( rect.z * 0.5f ) - ( text_width * 0.5f ), rect.y + offset + rect.w + 1, clr,
		awo::_render->font_tahoma, dist.c_str( ), awo::font_flags_t::outline);
}

void awo::esp_t::render_taser_range( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { 
	//if ( entity.player_pawn.weapon_type != awo::cs_weapon_type::weapon_type_taser ) {
	//	return; /* do not draw on anything else */
	//}

	// xref: 204ft
	//bone_pos head = local_player.get_bone( ).bone_pos_list[ bone_index::pelvis ];
	//
	//
	//_render->add_circle( head.screen_pos, 102, awo::col_t( ) );
}

#pragma warning ( disable :4996)
void awo::esp_t::change_smoke_color( uintptr_t ent ) {
	if ( !_settings->change_smoke )
		return; /* do not run bro */

	vec2_t screen;
	uintptr_t ent_base, adrr;
	char to_read[ 32 ];
	std::string class_name;

	/* read.memory */
	_proc_manager.read_memory<uintptr_t>( ( uintptr_t )ent, ent_base );
	_proc_manager.read_memory<uintptr_t>( ent_base + 0x10, adrr );
	_proc_manager.read_memory<uintptr_t>( adrr + 0x20, adrr );
	_proc_manager.read_memory<char[ 32 ]>( adrr, to_read );

	class_name = to_read;

#if 0
	static char prev_class_name[ 50 ] = ""; 
	if ( strcmp( class_name.c_str( ), prev_class_name ) != 0 ) {
		printf( "str -> %s \n", class_name );
		strcpy( prev_class_name, class_name.c_str( ) ); 
	}
#endif

	if ( class_name == "smokegrenade_projectile" ) {
		vec3_t smoke_coloringo = { _settings->smoke_coloringol[0] * 255.f, _settings->smoke_coloringol[ 1 ] * 255.f, _settings->smoke_coloringol[ 2 ] * 255.f };	
		_proc_manager.write_memory<vec3_t>( ent_base + offsets::c_base_smoke::smoke_color /* m_vSmokeColor  */, smoke_coloringo );
	}
}

void awo::esp_t::remove_smoke( uintptr_t ent ) {
	if ( !_settings->remove_smoke )
		return; /* do not run bro */

	uintptr_t ent_base;
	bool begin = false;
	int uf = 0; // 0x1108

	_proc_manager.read_memory<uintptr_t>( ( uintptr_t )ent, ent_base );
	_proc_manager.write_memory<bool>( ent_base + offsets::c_base_smoke::smoke_effect /* m_bDidSmokeEffect  */, begin );
	_proc_manager.write_memory<bool>( ent_base + offsets::c_base_smoke::effect_spawn /* m_bSmokeEffectSpawned  */, begin );
	_proc_manager.write_memory<int>( ent_base + offsets::c_base_smoke::effect_begin /* m_nSmokeEffectTickBegin  */, uf );

}

void awo::esp_t::render_incendiary( ) {
  /* thi shit is not workins */
	
}

#include "../shots/shots_hitsound.hpp"
void awo::esp_t::killed_by_hs( const c_entity& entity, int i ) {
	if ( !_settings->killedby_hs )
		return;

	/* vars */
	bone_pos head = entity.get_bone( ).bone_pos_list[ bone_index::head ];

	static bool has_been_killed_by_hs[ 64 ]{ false };
	bool was_hs;

	static bool play_sound{ false };
	static bool reseted{ false };

	/* read memory */
	_proc_manager.read_memory<bool>( entity.player_pawn.address + 0x1668, was_hs );

	/* target it */
	if ( was_hs ) {
		has_been_killed_by_hs[ i ] = true;
	}

#if 0 // meme
	if ( !play_sound && reseted ) {
		reseted = false;

		if ( _settings->hitsounduh ) {
			PlaySoundA( reinterpret_cast< char* > ( robloxsnd ), NULL, SND_ASYNC | SND_MEMORY );
		}

		play_sound = true;
	}

	if ( entity.player_pawn.health <= 0 && play_sound ) {
		play_sound = false;
		reseted = true;
	}
#endif
	auto animation = animation_controller.get( "killshot" + std::to_string( i ) + animation_controller.current_child );
	animation.adjust( animation.value + 3.f * animation_controller.get_min_deltatime( 0.2f ) * ( has_been_killed_by_hs[ i ] ? 1.f : -1.f ) );

	if ( animation.value >= 0.90 ) {
		has_been_killed_by_hs[ i ] = false;
	}

	std::string hs = "headshot";
	std::transform( hs.begin( ), hs.end( ), hs.begin( ), ::toupper );

	awo::_render->add_text( head.screen_pos.x, head.screen_pos.y,
							awo::col_t( 0, 255, 0 ).modify_alpha( 255 * animation.value ), awo::_render->font_tahoma, hs.c_str( ) );
}

void awo::esp_t::render_esp( const c_entity& local_player, const c_entity& entity, ImVec4 rect, int local_index, int index ) { 
	/* 
		dormant shit: 
			xref: entity.player_pawn.dormant

		spotted shit:
			xref: this->spotted( entity, local_player, local_index, index )
	*/


	if (!awo::_settings->visuals)
		return;
		
	/* setup dormancy */
	this->setup_alpha( local_player, entity, local_index, index ); 

	if ( awo::_settings->bounding_box ) {
		this->bounding_box( local_player, entity, rect, local_index, index );
	}

	if ( awo::_settings->name_esp ) {
		this->render_name( local_player, entity, rect, local_index, index );
	}

	if ( awo::_settings->health_bar ) {
		this->health_bar( local_player, entity, rect, local_index, index );
	}

	if ( awo::_settings->ammobar ) {
		this->ammo_bar( local_player, entity, rect, local_index, index );
	}

	if ( awo::_settings->eap ) {
		this->render_weapon( local_player, entity, rect, local_index, index );
	}

	/* we will leave this like that just because we might want head dot
		or skeleton
	*/
	this->skeleton_esp( local_player, entity, rect, local_index, index );
	this->render_distance( local_player, entity, rect, local_index, index );
	this->render_taser_range( local_player, entity, rect, local_index, index );

	if ( awo::_settings->flags ) {
		this->render_side_info( local_player, entity, rect, local_index, index );
	}
}

awo::macros::vec4_t awo::esp_t::get_player_bounding_box( const c_entity& entity ) {
	bone_pos head = entity.get_bone( ).bone_pos_list[ bone_index::head ];

	vec2_t size, pos;
	size.y = ( entity.player_pawn.screen_pos.y - head.screen_pos.y ) * 1.26;
	size.x = size.y * 0.6;

	// retarded 
	pos = vec2_t( entity.player_pawn.screen_pos.x - size.x / 2, head.screen_pos.y - size.y * 0.08 );

	return ImVec4{ pos.x + 2, pos.y - 5, size.x, size.y };
}