#define IMGUI_DEFINE_MATH_OPERATORS
#include "../inc.hpp"
#include <Uxtheme.h>
#include <dwmapi.h>
#include "../thirdparty/bytes.hpp"
#include "../thirdparty/custom.hpp"
#include "../sdk/render/render_fonts.hpp"
LRESULT wnd_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp ) {
    if ( ImGui_ImplWin32_WndProcHandler( hwnd, msg, wp, lp ) ) {
        return 0;
    }
    switch ( msg ) {
        case WM_SYSCOMMAND:
        {
            if ( ( wp & 0xfff0 ) == SC_KEYMENU )
                return 0;
            break;
        }

        case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            return 0;
        }
    }
    return DefWindowProc( hwnd, msg, wp, lp );
}


namespace awo::framework {
	bool render( ) {
        // get screen resolution
        m_i_width = GetSystemMetrics( SM_CXSCREEN );
        m_i_height = GetSystemMetrics( SM_CYSCREEN );

        if ( !m_b_initialized )
            return false;

        static bool bRunning = true;
        if ( bRunning ) {
            MSG message;
            while ( PeekMessage( &message, nullptr, 0U, 0U, PM_REMOVE ) ) {
                TranslateMessage( &message );
                DispatchMessage( &message );
            }
            if ( message.message == WM_QUIT )
                bRunning = false;

            static bool bToggled = false;
            if ( GetAsyncKeyState(VK_F1) & 1 && !bToggled ) {
                m_b_open = !m_b_open;
                SetForegroundWindow( instance );
                bToggled = true;
            } else if ( !GetAsyncKeyState( VK_F1 ) & 1 ) {
                LONG_PTR windowStyle = GetWindowLongPtr( instance, GWL_EXSTYLE );
                SetWindowLongPtr( instance, GWL_EXSTYLE, windowStyle | WS_EX_TRANSPARENT );
                bToggled = false;
            }

            if ( GetAsyncKeyState( VK_F2 ) & 1 ) {
                return 0;
            }

            ImGui_ImplDX11_NewFrame( );
            ImGui_ImplWin32_NewFrame( );

            ImGui::NewFrame( );

            if ( m_b_open ) {
                LONG_PTR windowStyle = GetWindowLongPtr( instance, GWL_EXSTYLE );
                windowStyle &= ~WS_EX_TRANSPARENT;
                SetWindowLongPtr( instance, GWL_EXSTYLE, windowStyle );

                _menu->render( );
            }

            awo::_hacks->run( );

            ImDrawList* pBackgroundDrawList = ImGui::GetBackgroundDrawList( );
            // Draw::RenderDrawData( pBackgroundDrawList ); rendering

            ImGui::Render( );

            constexpr float flColor[ 4 ] = { 0.f, 0.f, 0.f, 0.f };
            p_context->OMSetRenderTargets( 1U, &p_render_target_view, NULL );
            p_context->ClearRenderTargetView( p_render_target_view, flColor );

            ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

            p_swap_chain->Present( 1U, 0U );
        }

        if ( !bRunning ) {
            destroy( );
            return false;
        }

        return true;
    }

    bool create( ) {
        if ( m_b_initialized )
            return true;

        m_i_width = GetSystemMetrics( SM_CXSCREEN );
        m_i_height = GetSystemMetrics( SM_CYSCREEN );

        window_class.cbSize = sizeof( WNDCLASSEX );
        window_class.style = 0;
        window_class.lpfnWndProc = wnd_proc;
        window_class.hInstance = h_moudle;
        window_class.lpszClassName = ( L"awo-external" );

        RegisterClassExW( &window_class );

        instance = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
            window_class.lpszClassName, ( L"" ), WS_POPUP | WS_VISIBLE,
            0, 0, m_i_width, m_i_height, nullptr, nullptr, h_moudle, nullptr );

        if ( !SetLayeredWindowAttributes( instance, RGB( 0, 0, 0 ), BYTE( 255 ), LWA_ALPHA ) )
            throw std::runtime_error( ( "failed to set layered window attributes" ) );

        {
            RECT clientArea = { };
            if ( !GetClientRect( instance, &clientArea ) )
                throw std::runtime_error( ( "failed to get client rect" ) );

            RECT windowArea = { };
            if ( !GetWindowRect( instance, &windowArea ) )
                throw std::runtime_error( ( "failed to get window rect" ) );

            POINT diff = { };
            if ( !ClientToScreen( instance, &diff ) )
                throw std::runtime_error( ( "failed to get client to screen" ) );

            const MARGINS margins{
                windowArea.left + ( diff.x - windowArea.left ),
                windowArea.top + ( diff.y - windowArea.top ),
                windowArea.right,
                windowArea.bottom
            };

            if ( FAILED( DwmExtendFrameIntoClientArea( instance, &margins ) ) )
                throw std::runtime_error( ( "failed to extend frame into client area" ) );
        }

        // get refreshrate
        HDC hDC = GetDC( instance );
        m_u_refresh_rate = GetDeviceCaps( hDC, VREFRESH );
        ReleaseDC( instance, hDC );

        DXGI_SWAP_CHAIN_DESC swapChainDesc = { };
        swapChainDesc.BufferDesc.RefreshRate.Numerator = m_u_refresh_rate;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1U;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1U;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2U;
        swapChainDesc.OutputWindow = instance;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        constexpr D3D_FEATURE_LEVEL levels[ 2 ]
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0
        };

        D3D_FEATURE_LEVEL level = { };

        if ( D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0U,
            levels,
            2U,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &p_swap_chain,
            &p_device,
            &level,
            &p_context ) != S_OK )
            return false;

        ID3D11Texture2D* pBackBuffer = nullptr;
        p_swap_chain->GetBuffer( 0U, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
        if ( pBackBuffer ) {
            p_device->CreateRenderTargetView( pBackBuffer, nullptr, &p_render_target_view );
            pBackBuffer->Release( );
        } else throw std::runtime_error( ( "failed to get back buffer" ) );

        SetWindowLong( instance, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW );
        ShowWindow( instance, SW_SHOW );
        UpdateWindow( instance );

        ImGui::CreateContext( );
        ImGui::StyleColorsDark( );

        ImGui_ImplWin32_Init( instance );
        ImGui_ImplDX11_Init( p_device, p_context );

    
        _menu->initialize( );

        static bool o = false;
        if (!o) {
            _render->initialize_font_system();
            o = true;
        }


        printf( "[awo] created ui!\n" );

        m_b_initialized = true;
    }

    void destroy( ) {
        ImGui_ImplDX11_Shutdown( );
        ImGui_ImplWin32_Shutdown( );

        ImGui::DestroyContext( );

        if ( p_swap_chain )
            p_swap_chain->Release( );

        if ( p_context )
            p_context->Release( );

        if ( p_device )
            p_device->Release( );

        if ( p_render_target_view )
            p_render_target_view->Release( );

        if ( !DestroyWindow( instance ) )
            throw std::runtime_error( ( "failed to destroy window" ) );

        if ( !UnregisterClassW( window_class.lpszClassName, window_class.hInstance ) )
            throw std::runtime_error( ( "failed to destroy window" ) );
    }
}

#include <windows.h>
#include <Lmcons.h>



void awo::menu_t::render() {


    vector < const char* > items = { "Head", "Chest", "Body", "Legs", "Feet" };
    vector < const char* > cfgs = { "default", "legit", "auto", "scout", "other" };

    vector < const char* > animation_types = { "Left to right",
                                        "Middle pulse",
                                        "Tiny color" };

    vector < const char* > keymode = { "Hold",
                                    "Toggle",
                                    "Always" };

    vector < const char* > keymode2 = { "Prefer head",
                                "Prefer baim" };

    vector < const char* > stuff21 = { "Normal",
                            "Gradient" };

    vector < const char* > hitsound = {
                       "Neverlose",
                       "Skeet",
                       "Primordial",
                       "Sound 1",
                       "Sound 2",
                       "Toy duck",
                       "Bell 1",
                       "Bell 2",
                       "Button",
                       "Pop",
                       "Wow"
    };

    vector < const char* > hitboxes = { "Head",
                                "Neck",
                                "Chest", "Pelvis" };

    vector < const char* > key_binds = { "none", "mouse1", "mouse2", "mouse3", "mouse4", "mouse5", "a",
        "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
        "t", "u", "v", "w", "x", "y", "z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "f1",
        "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "alt" };


    using namespace ImGui;
    float SmoothMin = 0.1f, SmoothMax = 1.0f;

    static float alpha = 0.f, speed = 0.1f;
    alpha = ImLerp(alpha, awo::framework::m_b_open ? 1.f : 0.f, 1.f * speed);



    static bool checkbox[50];
    static bool multi[5];
    static int  slider = 0, combo = 0, combo_n = 0, key = 0, mode = 0;
    vector < const char* > combo_items = { "Item 1", "Item 2", "Item 3" };
    vector < const char* > multi_items = { "Item 1", "Item 2", "Item 3", "Item 4", "Item 5" };
    vector < const char* > combo_n_items = { "Enemy", "Friendly" };
    static vector < const char* > modes = { "Toggle", "Hold" };
    static char buf[64];
    static float color[4] = { 1.f, 1.f, 1.f, 1.f };

    static bool enableffi = false;
    static int  cur_lua = 0;
    static vector < const char* > lua_list = { "Weather", "Lua 1", "Lua 2", "Lua 3", "Lua 4", "Lua 5", "Lua 6", "Lua 7", "Lua 8", "Lua 9", "Lua 10", "Lua 11", "Lua 12" };

    static st_lua arr[50];


    ImGui::StyleColorsDark();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    if (awo::framework::m_b_open || alpha >= 0.05f) {

        ImGui::Begin("Menu", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
        {
            auto window = ImGui::GetCurrentWindow();
            auto draw = window->DrawList;
            auto pos = window->Pos;
            auto size = window->Size;
            auto style = ImGui::GetStyle();
            // 740 x 593
            // SetWindowSize( ImVec2( 620, 480 ) );
            ImGui::SetWindowSize(ImVec2(740 * alpha, 593 * alpha));

            custom.m_anim = ImLerp(custom.m_anim, 1.f, 0.038f);
            draw->AddText(GetIO().Fonts->Fonts[2], GetIO().Fonts->Fonts[2]->FontSize, pos + ImVec2(12, 12), ImColor(1.f, 1.f, 1.f, 0.8f * GetStyle().Alpha), "Aworex");

            char username[UNLEN + 1];
            DWORD username_len = UNLEN + 1;
            GetUserNameA(username, &username_len);

            draw->AddRectFilled(pos + ImVec2(0, 40), pos + ImVec2(size.x, 70), to_vec4(19, 20, 23, GetStyle().Alpha));
            draw->AddText(GetIO().Fonts->Fonts[3], GetIO().Fonts->Fonts[3]->FontSize, pos + ImVec2(10, 49), to_vec4(118, 132, 151, GetStyle().Alpha), "A");
            draw->AddText(GetIO().Fonts->Fonts[0], GetIO().Fonts->Fonts[0]->FontSize - 1, pos + ImVec2(27, 48), GetColorU32(ImGuiCol_Text, 0.5f), username);


            SetCursorPosX(size.x - custom.get_label_sizes(custom.tabs) - (40 * custom.tabs.size()));
            BeginGroup();

            for (int i = 0; i < custom.tabs.size(); ++i) {

                if (custom.tab(custom.tabs_icons.at(i), custom.tabs.at(i), custom.m_tab == i, i == custom.tabs.size() - 1 ? style.WindowRounding : 0, i == custom.tabs.size() - 1 ? ImDrawFlags_RoundCornersTopRight : 0) && custom.m_tab != i)
                    custom.m_tab = i, custom.m_anim = 0.f;

                if (i != custom.tabs.size() - 1)
                    SameLine();

            }

            EndGroup();

            switch (custom.m_tab) {

            case 0:

          
                SetCursorPos(ImVec2(size.x - custom.get_label_sizes(custom.aimbot_sub_tabs) - (GetStyle().ItemSpacing.x * custom.aimbot_sub_tabs.size()), 47));
                BeginGroup();

                for (int i = 0; i < custom.aimbot_sub_tabs.size(); ++i) {

                    if (custom.sub_tab(custom.aimbot_sub_tabs.at(i), custom.m_aimbot_sub_tab == i) && custom.m_aimbot_sub_tab != i)
                        custom.m_aimbot_sub_tab = i, custom.m_anim = 0.f;

                    if (i != custom.aimbot_sub_tabs.size() - 1)
                        SameLine();

                }

                EndGroup();

                switch (custom.m_aimbot_sub_tab)
                {
                case 0:


                    SetCursorPos(ImVec2(10, 80));
                    BeginChild("##aimbot.groupboxes.area", ImVec2(size.x - 20, size.y - 90)); {

                        custom.group_box_alternative("##master_switch.aimbot", ImVec2(GetWindowWidth(), 53)); {

                            custom.checkbox("Master switch##aimbot", &checkbox[3], "Toggle on all aimbot features", GetIO().Fonts->Fonts[1]);

                        } custom.end_group_box_alternative();

                        custom.group_box("General", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y)); {

                            ImGui::Checkbox("Draw fov", &awo::_settings->draw_fov);
                            ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                            ImGui::ColorEdit4("###foviclo", awo::_settings->fov_color, ALPHA);

                            ImGui::Checkbox("Only visible", &awo::_settings->visible_check);
                            ImGui::Combo("Aimbot key", &awo::_settings->a_triggerkey, key_binds.data(), key_binds.size());
                            ImGui::Combo("Aimbot mode", &awo::_settings->a_activationz_type, keymode.data(), keymode.size());
                            ImGui::Combo("Hitbox ", &awo::_settings->hitbox, hitboxes.data(), hitboxes.size());
                            ImGui::Checkbox("Body-aim if lethal", &awo::_settings->legitbot_stuff[9]);

                            // ImGui::SliderInt( "Max distance", &awo::_settings->aim_distance_max, 200, 10000 );

                            ImGui::Checkbox("Dynamic fov", &awo::_settings->legitbot_stuff[0]);
                            ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                            if (custom.settings_widget("##aimbot.enable.settings")) OpenPopup("##aimbot.enable.popup");
                            custom.prepared_popup("##aimbot.enable.popup", "Dynamic fov", []() {
                                ImGui::Checkbox("Enemy moving", &awo::_settings->legitbot_stuff[1]);
                                ImGui::Checkbox("Enemy lethal", &awo::_settings->legitbot_stuff[2]);
                                ImGui::Checkbox("Local lethal", &awo::_settings->legitbot_stuff[3]);
                                ImGui::Checkbox("Dealt alot of damage", &awo::_settings->legitbot_stuff[7]);
                                if (awo::_settings->legitbot_stuff[7]) {
                                    ImGui::SliderInt("Dealt damage", &awo::_settings->legitbot_stuff_int[0], 100, 500);
                                }

                                });

                            ImGui::Checkbox("Dynamic smooth", &awo::_settings->legitbot_stuff[4]);
                            ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                            if (custom.settings_widget("##popup2")) OpenPopup("##popup2");
                            custom.prepared_popup("##popup2", "Dynamic smooth", []() {
                                ImGui::Checkbox("Enemy moving", &awo::_settings->legitbot_stuff[5]); // 0.5
                                ImGui::Checkbox("Local lethal", &awo::_settings->legitbot_stuff[6]); // 0.5
                                ImGui::Checkbox("Dealt alot of damage", &awo::_settings->legitbot_stuff[8]);
                                if (awo::_settings->legitbot_stuff[8]) {
                                    ImGui::SliderInt("Dealt damage", &awo::_settings->legitbot_stuff_int[1], 100, 500);
                                }
                                });


                            ImGui::SliderFloat("Fov", &awo::_settings->fov, 0, 10);
                            ImGui::SliderFloat("Smooth", &awo::_settings->smooth, 0, 10);




                        }


                    }
                    custom.end_group_box();

                    ImGui::SameLine();

                    custom.group_box("Other", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y));
                    {
                        ImGui::Checkbox("Triggerbot", &awo::_settings->triggerbot);
                        ImGui::Combo("Triggerbot key", &awo::_settings->triggerkey, key_binds.data(), key_binds.size());
                        ImGui::Combo("Triggerbot mode", &awo::_settings->activationz_type, keymode.data(), keymode.size());

                        ImGui::SliderInt("Next reaction time", &awo::_settings->reaction_time, 200, 2500);
                        ImGui::SliderInt("Next shot delay", &awo::_settings->shot_delay, 10, 2500);

                    } custom.end_group_box();

                    EndChild();
                    break;


                case 1:
                {

                    SetCursorPos(ImVec2(10, 80));
                    BeginChild("##ragebot.groupboxes.area", ImVec2(size.x - 20, size.y - 90)); {

                        custom.group_box_alternative("##master_switch.ragebot", ImVec2(GetWindowWidth(), 53)); {

                            custom.checkbox("Master switch##ragebot", &awo::_settings->rage, "Toggle on all ragebot features", GetIO().Fonts->Fonts[1]);

                        } custom.end_group_box_alternative();

                        custom.group_box("General", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y)); {
                            ImGui::Checkbox("Ignore wall", &awo::_settings->ignore_wall);
                            // ImGui::Combo( "Hitbox ", &awo::_settings->rage_hitbox, hitboxes.data( ), hitboxes.size( ) );
                            ImGui::Checkbox("Ignore if dist is over max", &awo::_settings->ignore_if_Distance_tO_high);
                            if (awo::_settings->ignore_if_Distance_tO_high)
                                ImGui::SliderInt("Max aim distance", &awo::_settings->distance_to_rage, 200, 3000);

                            std::string name_str = awo::_settings->rage_fov > 10 ? "Fov (!)" : "Fov";
                            ImGui::SliderInt("Fov", &awo::_settings->rage_fov, 1, 20);



                        }


                    }
                    custom.end_group_box();

                    ImGui::SameLine();

                    custom.group_box("Other", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y));
                    {
                        ImGui::Checkbox("Multipoint", &awo::_settings->ragebot_stuff[0]);
                        ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                        if (custom.settings_widget("##popup3")) OpenPopup("##popup3");
                        custom.prepared_popup("##popup3", "Multipoint", [&]() {
                            ImGui::Combo("Prefer..", &awo::_settings->ragebot_stuff2[0], keymode2.data(), keymode2.size());
                            ImGui::Checkbox("Baim while lethal", &awo::_settings->ragebot_stuff[1]);
                            ImGui::Checkbox("Baim unsafe", &awo::_settings->ragebot_stuff[2]);

                            });

                    } custom.end_group_box();

                    EndChild();

                    break;
                }
                }
            break;

            break;

            case 1:
            {


                SetCursorPos(ImVec2(10, 80));
                BeginChild("##visuals.area", ImVec2(size.x - 20, size.y - 90)); {

                    custom.group_box_alternative("##master_switch.visuals", ImVec2(GetWindowWidth(), 53)); {

                        custom.checkbox("Master switch##visuals", &awo::_settings->visuals, "Toggle on all visual features", GetIO().Fonts->Fonts[1]);

                    } custom.end_group_box_alternative();

                    custom.group_box("General", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y)); {

                      
                        ImGui::Checkbox("Spotted esp", &awo::_settings->change_by_visibility);
                        ImGui::Checkbox("Dormancy esp", &awo::_settings->dormancy);
                        ImGui::Checkbox("Bounding box", &awo::_settings->bounding_box);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        ImGui::ColorEdit4("###boxcolk", awo::_settings->box_color, ALPHA);
                        if (awo::_settings->change_by_visibility) {
                            ImGui::SameLine(GetWindowWidth() - 73);
                            ImGui::ColorEdit4("###boxcolkspot", awo::_settings->box_color_inv, ALPHA);
                        }



                        ImGui::Checkbox("Name", &awo::_settings->name_esp);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        ImGui::ColorEdit4("###namecls", awo::_settings->name_color, ALPHA);
                        if (awo::_settings->change_by_visibility) {
                            ImGui::SameLine(GetWindowWidth() - 73);
                            ImGui::ColorEdit4("###namceks", awo::_settings->name_color_inv, ALPHA);
                        }

                        if (awo::_settings->name_esp) {
                            ImGui::Checkbox("Name animation", &awo::_settings->name_animation);
                            ImGui::SameLine(GetWindowWidth() - 58);
                            ImGui::ColorEdit4("###nameanimcl", awo::_settings->name_color_a, ALPHA);

                            ImGui::Combo("Name animation type", &awo::_settings->name_at, animation_types.data(), animation_types.size());
                        }

                        ImGui::Checkbox("Health bar", &awo::_settings->health_bar);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        if (custom.settings_widget("##popup7")) OpenPopup("##popup7");
                        custom.prepared_popup("##popup7", "Health bar", [&]() {
                            ImGui::Checkbox("Custom color", &awo::_settings->customhealthbar);
                            ImGui::SameLine(GetWindowWidth() - 55);
                            ImGui::ColorEdit4("###healthbari", awo::_settings->healthbar, ALPHA);
                            if (awo::_settings->change_by_visibility) {
                                ImGui::SameLine(GetWindowWidth() - 45);
                                ImGui::ColorEdit4("###heabarb", awo::_settings->healthbari, ALPHA);
                            }
                            ImGui::Combo("Color type", &awo::_settings->visuals_i[0], stuff21.data(), stuff21.size());
                            if (awo::_settings->visuals_i[0] == 1) {
                                ImGui::Text("Second color");
                                ImGui::SameLine(GetWindowWidth() - 45);
                                ImGui::ColorEdit4("###healthbari22", awo::_settings->visuals_c[0], ALPHA);
                            }

                            });



                        ImGui::Checkbox("Ammo bar", &awo::_settings->ammobar);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        ImGui::ColorEdit4("###ammobar", awo::_settings->ammobar_color, ALPHA);
                        if (awo::_settings->change_by_visibility) {
                            ImGui::SameLine(GetWindowWidth() - 73);
                            ImGui::ColorEdit4("###bajsdggqd", awo::_settings->ammobar_color_inv, ALPHA);
                        }
                        ImGui::SameLine(GetWindowWidth() - 86);
                        if (custom.settings_widget("##popup8")) OpenPopup("##popup8");
                        custom.prepared_popup("##popup8", "Ammo bar", [&]() {
                            ImGui::Combo("Color type", &awo::_settings->visuals_i[1], stuff21.data(), stuff21.size());
                            if (awo::_settings->visuals_i[1] == 1) {
                                ImGui::Text("Second color");
                                ImGui::SameLine(GetWindowWidth() - 44);
                                ImGui::ColorEdit4("###healthbar123i22", awo::_settings->visuals_c[1], ALPHA);
                            }

                            });



                        ImGui::Checkbox("Distance", &awo::_settings->visuals_b[0]);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        ImGui::ColorEdit4("###wepoasdasdsansadhg", awo::_settings->visuals_c[2], ALPHA);

                        ImGui::Checkbox("Weapon name", &awo::_settings->eap);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        ImGui::ColorEdit4("###weponsadhg", awo::_settings->eap_color, ALPHA);
                        if (awo::_settings->change_by_visibility) {
                            ImGui::SameLine(GetWindowWidth() - 73);
                            ImGui::ColorEdit4("###weapon12hsa", awo::_settings->eap_color_inv, ALPHA);
                        }

                        if (awo::_settings->eap) {
                            ImGui::Checkbox("Weaon name animation", &awo::_settings->weapon_animation);
                            ImGui::SameLine(GetWindowWidth() - 58);
                            ImGui::ColorEdit4("###nameanimcl", awo::_settings->weapon_color_a, ALPHA);

                            ImGui::Combo("Weaon name animation type", &awo::_settings->weapon_at, animation_types.data(), animation_types.size());

                        }
                        ImGui::Checkbox("Head bone", &awo::_settings->bones_h);
                        ImGui::Checkbox("Skeleton", &awo::_settings->bones);
                        ImGui::SameLine(GetWindowWidth() - 58);
                        ImGui::ColorEdit4("###bonesh12", awo::_settings->bone_color, ALPHA);
                        if (awo::_settings->change_by_visibility) {
                            ImGui::SameLine(GetWindowWidth() - 73);
                            ImGui::ColorEdit4("###onesi12", awo::_settings->bone_color_inv, ALPHA);
                        }

                        ImGui::Checkbox("Flags", &awo::_settings->flags);
                        ImGui::Checkbox("Show competitive wins", &awo::_settings->show_competivie_wins);
                        ImGui::Checkbox("Show dealt damage that round", &awo::_settings->show_dmg_dealt);

                       
                    }


                }
                custom.end_group_box();

                ImGui::SameLine();

                custom.group_box("Other", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y));
                {
                  


                    ImGui::Checkbox("Colored smoke (!)", &awo::_settings->change_smoke);
                    ImGui::SameLine(GetWindowWidth() - 58);
                    ImGui::ColorEdit4("###smokecolori", awo::_settings->smoke_coloringol, ALPHA);

                    ImGui::Checkbox("Remove smoke (!)", &awo::_settings->remove_smoke);
                    ImGui::Checkbox("Flashbang builder (!)", &awo::_settings->flash_builder);
                    if (!_settings->remove_full_flash) {
                        ImGui::SameLine(GetWindowWidth() - 33);
                        if (custom.settings_widget("##popup6")) OpenPopup("##popup6");
                        custom.prepared_popup("##popup6", "Flashbang builder (!)", [&]() {
                            ImGui::SliderFloat("Flashbang delay (5<)", &awo::_settings->flash_time, 0.f, 5.f);
                            ImGui::SliderFloat("Flashbang alpha", &awo::_settings->flash_alpha, 0.f, 255.f);


                            });
                    }

                    if (awo::_settings->flash_builder)
                        ImGui::Checkbox("Remove flash completly (!)", &awo::_settings->remove_full_flash);


                    ImGui::Checkbox("Killed by headshot", &awo::_settings->killedby_hs);
                    ImGui::Checkbox("Hitmarker", &awo::_settings->hitmarker);
                    ImGui::SameLine(GetWindowWidth() - 58);
                    ImGui::ColorEdit4("###hitmarkecol", awo::_settings->hitmarker_col, ALPHA);

                    ImGui::Checkbox("Hitsound", &awo::_settings->hitsound);
                    if (awo::_settings->hitsound)
                        ImGui::Combo("Sound", &awo::_settings->hitsound_type, hitsound.data(), hitsound.size());

                    ImGui::Checkbox("Local sound esp", &awo::_settings->local_sound);
                    ImGui::SameLine(GetWindowWidth() - 78);
                    if (custom.settings_widget("##popup4")) OpenPopup("##popup4");
                    custom.prepared_popup("##popup4", "Local sound esp", [&]() {
                        ImGui::SliderFloat("Animation###local", &awo::_settings->sound_animation_speed_l, 0.1, 0.3f);
                        ImGui::SliderInt("Sound range", &awo::_settings->local_range, 10, 100);

                        });
                    ImGui::SameLine(GetWindowWidth() - 55);
                    ImGui::ColorEdit4("###solc1231231ol", awo::_settings->sound_col_l, ALPHA);



                    ImGui::Checkbox("Enemy sound esp", &awo::_settings->enemy_sound);
                    ImGui::SameLine(GetWindowWidth() - 78);
                    if (custom.settings_widget("##popup5")) OpenPopup("##popup5");
                    custom.prepared_popup("##popup5", "Enemy sound esp", [&]() {
                        ImGui::SliderFloat("Animation###enemy", &awo::_settings->sound_animation_speed_e, 0.1, 0.3f);
                        ImGui::SliderInt("Sound range###enemysound", &awo::_settings->enemy_range, 10, 100);
                        });

                    ImGui::SameLine(GetWindowWidth() - 55);
                    ImGui::ColorEdit4("###solcol3e", awo::_settings->sound_col_e, ALPHA);


                } custom.end_group_box();

                EndChild();
                break;
            }
                      /*  custom.checkbox("Enable", &checkbox[0], "Hint"); SameLine(GetWindowWidth() - 64); if (custom.settings_widget("##aimbot.enable.settings")) OpenPopup("##aimbot.enable.popup"); SameLine(GetWindowWidth() - 47); ColorEdit4("##color", color, ALPHA);

                        custom.prepared_popup("##aimbot.enable.popup", "Enable", []() {

                            Checkbox("Checkbox 2", &checkbox[2]);
                            SliderInt("Slider", &slider, 0, 100);
                            Hotkey("Key", &key);
                            Combo("Mode", &mode, modes.data(), modes.size());

                            });

                        Checkbox("Checkbox 1", &checkbox[1]);
                        SliderInt("Slider", &slider, 0, 100);
                        Combo("Combo", &combo, combo_items.data(), combo_items.size());
                        InputText("Input", buf, sizeof buf);
                        Button("Button", ImVec2(GetWindowWidth() - 25, 25));
                        MultiCombo("Multi", multi, multi_items.data(), multi_items.size());
                        SliderFloat("Speed", &speed, 0.05f, 1.f);

                    } custom.end_group_box();

                    SameLine();

                    custom.group_box("Miscellaneous", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y)); {



                    } custom.end_group_box();

                } EndChild();*/
            case 3:

                SetCursorPos(ImVec2(10, 80));
                BeginChild("##scripts.groupboxes.area", ImVec2(size.x - 20, size.y - 90)); {

                    custom.group_box_alternative("##enable_ffi.scripts", ImVec2(GetWindowWidth(), 53)); {

                        custom.checkbox("Enable Foreign Function Interface##scripts", &enableffi, "Allow scripts to use FFI.", GetIO().Fonts->Fonts[1]);
                        SameLine(GetWindowWidth() - 155);
                        custom.button("Get more scripts", "B", ImVec2(130, 30));

                    } custom.end_group_box_alternative();

                    custom.list_box("Available scripts", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2 - 70, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y), []() {

                        for (int i = 0; i < lua_list.size(); ++i) {

                            arr[i] = custom.lua(lua_list.at(i), cur_lua == i);

                            if (arr[i].pressed_whole)
                                cur_lua = i;
                        }

                        }, ImVec2(0, 0));

                    SameLine(), SetCursorPosY(53 + GetStyle().ItemSpacing.y);

                    custom.group_box_alternative("##lua.information", ImVec2(GetWindowWidth() / 2 + 66, GetWindowHeight() - 53 - GetStyle().ItemSpacing.y), ImVec2(1, 1)); {


                    } custom.end_group_box_alternative();

                } EndChild();

                break;

            }

        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

}


void awo::menu_t::initialize( ) { 
   
    ImGuiIO& io = ImGui::GetIO( ); ( void )io;
   

    ImFontConfig font_config;
    font_config.PixelSnapH = false;
    font_config.FontDataOwnedByAtlas = false;
    font_config.OversampleH = 5;
    font_config.OversampleV = 5;
    font_config.RasterizerMultiply = 1.2f;

    static const ImWchar ranges[] = {

        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0xE000, 0xE226, // icons
        0,
    };

    font_config.GlyphRanges = ranges;

    // MENU //
    io.Fonts->AddFontFromMemoryTTF(font_medium_binary, sizeof font_medium_binary, 15, &font_config, ranges); // 0
    io.Fonts->AddFontFromMemoryTTF(font_semi_binary, sizeof font_semi_binary, 15, &font_config, ranges); // 1
    io.Fonts->AddFontFromMemoryTTF(font_bold_binary, sizeof font_bold_binary, 19, &font_config, ranges); // 2

    io.Fonts->AddFontFromMemoryTTF(icons_binary, sizeof icons_binary, 9, &font_config, ranges); // 3
    io.Fonts->AddFontFromMemoryTTF(icons_binary, sizeof icons_binary, 13, &font_config, ranges); // 4

    io.Fonts->AddFontFromMemoryTTF(font_semi_binary, sizeof font_semi_binary, 17, &font_config, ranges); // 5
    // MENU //


    // MAIN // 


 
    // MAIN // 
      


}
