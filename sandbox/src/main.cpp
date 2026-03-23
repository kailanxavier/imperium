#include <core/core.h>
#include <fwk/fwk.h>
#include <iostream>

int main()
{
	std::cout << "Core: " << imp::core::Version() << std::endl;

	return 0;
}