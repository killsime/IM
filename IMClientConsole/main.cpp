#include "NetworkClient.hpp"
#include "ConsoleUI.hpp"

int main()
{
	NetworkClient networkClient("127.0.0.1", 9527, 9528);
	if (!networkClient.init())
		return 1;

	ConsoleUI consoleUI(networkClient);
	consoleUI.start();

	return 0;
}