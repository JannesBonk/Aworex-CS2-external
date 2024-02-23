#include "../../../inc.hpp"
#include "../../../thirdparty/imgui_internal.h"

void release_mouse_event_rage( ) {
    /* we can now shot so lets do it */
    std::this_thread::sleep_for( std::chrono::milliseconds( awo::_settings->shot_delay ) ); /* add a custom delay and more */
    mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 ); /* release shot event */
}


void awo::rage_t::run_aimbot( const c_entity& entity, const c_entity& local, vec3_t local_pos, int ent_idx, int local_idx ) {
    // gheto ass pasted - aimstar with modifications
    if ( !_settings->rage )
        return;
    
   
    /* hacks_ctx.cpp */
    float distance_to_sight = 0;
    float max_aim_distance = 1500;
    vec3_t aim_pos{ 0, 0, 0 };

    float distance_aim = local.player_pawn.vec_origin.dist_to( entity.player_pawn.vec_origin );
    if ( _settings->ignore_if_Distance_tO_high ) {
        if ( distance_aim > _settings->distance_to_rage ) {
            return; /* do not try for higher distance */
        }
    }

    // multipoint [0]
    bool b1 = awo::_settings->ragebot_stuff[ 1 ] && entity.player_pawn.health < 30;
    bool entity_lol = entity.player_pawn.health > local.player_pawn.health;
    bool b2 = awo::_settings->ragebot_stuff[ 2 ] && entity_lol;

    if ( _settings->ragebot_stuff[ 0 ] ) { /* thats the multipoint bruther */

        if ( !b1 || !b2 ) {
            switch ( _settings->ragebot_stuff2[ 0 ] ) {
                case 0:
                {
                    this->aim_position = bone_index::head;
                } break;
                case 1:
                {
                    this->aim_position = bone_index::pelvis;
                } break;
            }
        }

        if ( b1 || b2 ) {
            this->aim_position = bone_index::pelvis;
        }
       
    }

    /* hitbox shit */
    switch ( awo::_settings->rage_hitbox ) {
        case 0:
        {
            this->aim_position = bone_index::head;
        } break;
        case 1:
        {
            this->aim_position = bone_index::neck_0;
        } break;
        case 2:
        {
            this->aim_position = bone_index::spine_1;
        } break;
        case 3:
        {
            this->aim_position = bone_index::pelvis;
        } break;
    }

    distance_to_sight = entity.get_bone( ).bone_pos_list[ bone_index::head ].screen_pos.dist_to( { ( 1920 / 2 ), ( 1080 / 2 ) } );
    if ( distance_to_sight < max_aim_distance ) {
        max_aim_distance = distance_to_sight;

        if ( !_settings->ignore_wall || entity.player_pawn.spotted_by_mask & ( DWORD64( 1 ) << ( local_idx ) ) || local.player_pawn.spotted_by_mask & ( DWORD64( 1 ) << ( ent_idx ) ) ) {
            aim_pos = entity.get_bone( ).bone_pos_list[ this->aim_position ].pos;

            if ( this->aim_position == bone_index::head )
                aim_pos.z -= 1.f;
        }
    }

    /* paste fix */
    if ( aim_pos == vec3_t( 0, 0, 0 ) ) {
        return;
    }

    if ( framework::m_b_open ) {
        return;
    }

    if ( entity.player_pawn.health <= 0 ) {
        return;
    }

    float yaw, pitch, norm;
    vec3_t offset;
    vec2_t angles{ 0, 0 };
    int centerX = 1920 / 2, centerY = 1080 / 2;

    offset = aim_pos - local_pos;
    float distance = sqrt(pow(offset.x, 2) + pow(offset.y, 2));
    float length = sqrt(distance * distance + offset.z * offset.z);

   
        auto oldPunch = vec2_t{};

        if (local.player_pawn.shots_fired) {
            auto& viewAngles = local.player_pawn.viewangle;
            auto& aimPunch = local.player_pawn.aim_punch_angle;

            auto newAngles = vec2_t{
                viewAngles.x + oldPunch.x - aimPunch.x * 2.f,
                viewAngles.y + oldPunch.y - aimPunch.y * 2.f,
            };

            newAngles.x = std::clamp(newAngles.x, -89.f, 89.f);
            newAngles.y = std::fmod(newAngles.y + 180.f, 360.f) - 180.f;

            newAngles.x += centerX;
            newAngles.y += centerY;

            angles = newAngles;
        }
        else {
            oldPunch.x = oldPunch.y = 0.f;
        }

        vec2_t punchAngle;
        if (auto& cache = local.player_pawn.aim_punch_cache;
            cache.count > 0 && cache.count <= 0xFFFF &&
            _proc_manager.read_memory<vec2_t>(cache.data + (cache.count - 1) * sizeof(vec3_t), punchAngle)) {
            angles = punchAngle;
        }
        float radX = angles.x * RCSScale.x / 180.f * IM_PI;
        float radY = -angles.y * RCSScale.y / 180.f * IM_PI;
        float siX = sinf(radX), coX = cosf(radX), siY = sinf(radY), coY = cosf(radY);

        float z = offset.z * coX + distance * siX, d = (distance * coX - offset.z * siX) / distance;
        float x = (offset.x * coY - offset.y * siY) * d, y = (offset.x * siY + offset.y * coY) * d;

        offset = vec3_t{ x, y, z };

        aim_pos = local_pos + offset;
    

    yaw = atan2f(offset.y, offset.x) * 57.295779513 - local.player_pawn.viewangle.y;
    pitch = -atan(offset.z / distance) * 57.295779513 - local.player_pawn.viewangle.x;
    norm = sqrt(pow(yaw, 2) + pow(pitch, 2));

    vec2_t screenPos;

    _address->view.world_to_screen(vec3_t(aim_pos), screenPos);


    if (norm < _settings->rage_fov * 1.4) {
        int x = 1920 / 2;
        int y = 1080 / 2;
        vec2_t sp;
        _address->view.world_to_screen(aim_pos, sp);

        int tx = sp.x - x;
        int ty = sp.y - y;

        if (tx != 0 || ty != 0) {
            int stx = static_cast<int>(tx * (1.0f - 1.0f / 100));
            int sty = static_cast<int>(ty * (1.0f - 1.0f / 100));
            mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(stx), static_cast<DWORD>(sty), NULL, NULL);
        }
    }
}

    ///* fix */
    //opp_pos = aim_pos - local_pos;
    //distance = sqrt( pow( opp_pos.x, 2 ) + pow( opp_pos.y, 2 ) );
    //lenght = sqrt( distance * distance + opp_pos.z * opp_pos.z );

    //yaw = atan2f( opp_pos.y, opp_pos.x ) * 57.295779513 - local.player_pawn.viewangle.y;
    //pitch = -atan( opp_pos.z / distance ) * 57.295779513 - local.player_pawn.viewangle.x;
    //norm = sqrt( pow( yaw, 2 ) + pow( pitch, 2 ) );

    //vec2_t screen_pos;
    //_address->view.world_to_screen( vec3_t( aim_pos ), screen_pos );

    //if ( norm < _settings->rage_fov ) {
    //    if ( screen_pos.x != screen_center_x ) {
    //        target_x = ( screen_pos.x > screen_center_x ) ? -( screen_center_x - screen_pos.x ) : screen_pos.x - screen_center_x;
    //       // target_x /= 0.5f;
    //        target_x = ( target_x + screen_center_x > screen_center_x * 2 || target_x + screen_center_x < 0 ) ? 0 : target_x;
    //    }

    //    if ( screen_pos.y != 0 ) {
    //        if ( screen_pos.y != screen_center_y ) {
    //            target_y = ( screen_pos.y > screen_center_y ) ? -( screen_center_y - screen_pos.y ) : screen_pos.y - screen_center_y;
    //           // target_y /= 0.5f;
    //            target_y = ( target_y + screen_center_y > screen_center_y * 2 || target_y + screen_center_y < 0 ) ? 0 : target_y;
    //        }
    //    }

    //    /* should be much better now */
    //    if ( ( target_x < 2 && target_y < 2 ) && _esp->l_spotted( entity, local, local_idx, ent_idx ) ) {
    //        mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );
    //        std::thread trigger_thread( release_mouse_event_rage );
    //        trigger_thread.detach( );
    //    }

   
    //    float distance_ratio = norm / _settings->rage_fov;
    //    float speed_factor = 1.0f + ( 1.0f - distance_ratio );
    //    target_x /= speed_factor;
    //    target_y /= speed_factor;

    //    if ( screen_pos.x != screen_center_x ) {
    //        target_x = ( screen_pos.x > screen_center_x ) ? -( screen_center_x - screen_pos.x ) : screen_pos.x - screen_center_x;
    //       // target_x /= 0.5f;
    //        target_x = ( target_x + screen_center_x > screen_center_x * 2 || target_x + screen_center_x < 0 ) ? 0 : target_x;
    //    }

    //    if ( screen_pos.y != 0 ) {
    //        if ( screen_pos.y != screen_center_y ) {
    //            target_y = ( screen_pos.y > screen_center_y ) ? -( screen_center_y - screen_pos.y ) : screen_pos.y - screen_center_y;
    //           // target_y /= 0.5f;
    //            target_y = ( target_y + screen_center_y > screen_center_y * 2 || target_y + screen_center_y < 0 ) ? 0 : target_y;
    //        }
    //    }
    //    mouse_event( MOUSEEVENTF_MOVE, target_x, target_y, NULL, NULL );
    //  
    //}


