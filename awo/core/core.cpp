#define WIN32_LEAN_AND_MEAN 

#include "../inc.hpp"

int main( ) {
	SetConsoleTitle( L"cs2-dev" );

	auto process_status = _proc_manager.attach( "cs2.exe" );

	switch ( process_status ) {
		case 1:
			goto END;
		case 2:
			goto END;
		case 3:
			goto END;
		default:
			break;
	}

	if ( !awo::framework::m_b_initialized )
		awo::framework::create( );

	if ( !awo::_interfaces->initialize( ) ) {
		printf( "[awo] failtd to init offsets\n" );
		goto END;
	}

	printf( "[awo] initialized interfaces!\n" );

	if ( !awo::_address->initialize( ) ) {
		printf( "[awo] failtd to init addresses\n" );
		goto END;
	}

	while ( !awo::framework::unloading ) {
		if ( !awo::framework::render( ) )
			return 0;

		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}

END:
	std::cout << std::endl;
	system( "pause" );
	return 0;
}