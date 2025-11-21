#include "core/application.h"

std::unique_ptr<platformer2d::CApplication> Application;

int main(int Argc, char* Argv[])
{
	Application = std::make_unique<platformer2d::CApplication>(Argc, Argv);

	Application->Initialize();
	Application->Run();
	Application->Shutdown();

	LK_INFO_TAG("Main", "Exit");
	return 0;
}
