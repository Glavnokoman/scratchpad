#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

TEST_CASE("test something", "[test_smth]"){
	REQUIRE(2 + 2 == 4);
}

int main( int argc, char* argv[] )
{
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return ( result < 0xff ? result : 0xff );
}
