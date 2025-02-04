#include "Core/Engine.h"



int main(int argc, char** argv) 
{
    MamontEngine::MEngine Engine;
    Engine.Init();
    Engine.Run();
    Engine.Cleanup();

    return 0;
}