#pragma once

namespace awo {
	enum fonts_t {
		_default,
		_default_2,
		_default_3,
		_default_4,
		_default_5,
		_default_6,
		_default_7,
		_default_8

	};



	enum font_flags_t {
		none,
		dropshadow,
		outline
	};

	enum text_animation_t {
		left_to_right,
		middle_pulse,
		tiny_color
	};

	class render_t {
	public:
		void initialize_font_system( );

		void add_line( awo::vec2_t from, awo::vec2_t to, col_t c, float thickness );

		void add_text( int x, int y, col_t c, ImFont* font, const char* text, int /* or font_flags_t */ flag = font_flags_t::none );
		void add_rect_filled( int x, int y, int w, int h, awo::col_t col, int round );
		void add_rect( int x, int y, int w, int h, awo::col_t col, int round, int tickness );
		awo::col_t to_main_color( float color[ 4 ] );
		void add_filled_circle( awo::vec2_t center, float radius, awo::col_t c );
		void add_circle( awo::vec2_t center, float radius, awo::col_t c );
		void add_gradient_circle_filled( awo::vec2_t c, int radius, awo::col_t inner_color, awo::col_t outer_color );
		void add_gradient_vertical( int x, int y, int w, int h, awo::col_t c1, awo::col_t c2 );
		void add_gradient_horizontal( int x, int y, int w, int h, awo::col_t c1, awo::col_t c2 );
		void add_drop_shadow( int x, int y, int w, int h );
		void add_image( int x, int y, int w, int h, awo::macros::texture_id user_texture_id, awo::col_t c );

		void bind_animation( int id, std::string text, awo::col_t color, ImFont* font, int x, int y, text_animation_t animation_type, float animation_speed );

		void clip_rect( float x, float y, float w, float h, std::function<void( )> function );

		awo::vec2_t text_size( const char* text, ImFont* font );
		ImFont* font_tahoma;
		ImFont* font_small_pixel;
		std::vector<ImFont*> fonts{};
	};

	inline const auto _render = std::make_unique<render_t>( );
}