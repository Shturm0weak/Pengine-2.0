#include "Core/EntryPoint.h"
#include "EditorApplication.h"
#include "Core/Logger.h"

#include "../Include/Components.h"

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
		EditorApplication application{};
		Pengine::EntryPoint entryPoint(&application);
		entryPoint.Run();
	}
	catch (const std::runtime_error& runtimeError)
	{
		Pengine::Logger::Error(runtimeError.what());
	}

	return 0;
}
