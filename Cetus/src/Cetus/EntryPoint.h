#pragma once

extern Cetus::Application* Cetus::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

namespace Cetus {

	int Main(int argc, char** argv)
	{
		while (g_ApplicationRunning)
		{
			Cetus::Application* app = Cetus::CreateApplication(argc, argv);
			app->Run();
			delete app;
		}

		return 0;
	}

}

int main(int argc, char** argv)
{
	return Cetus::Main(argc, argv);
}
