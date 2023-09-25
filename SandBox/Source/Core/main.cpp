#include "Core/EntryPoint.h"
#include "ExampleApplication.h"

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
//{
//	try
//	{
//		ExampleApplication application;
//		Pengine::EntryPoint entryPoint(&application);
//		entryPoint.Run();
//	}
//	catch (const std::runtime_error& runtimeError)
//	{
//		__debugbreak();
//	}
//
//	return 0;
//}

int main()
{
	try
	{
		ExampleApplication application;
		Pengine::EntryPoint entryPoint(&application);
		entryPoint.Run();
	}
	catch (const std::runtime_error& runtimeError)
	{
		__debugbreak();
	}

	return 0;
}
