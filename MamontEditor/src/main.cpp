#include "Core/Engine.h"
#include "Editor.h"


int main(int argc, char** argv) 
{
    MamontEngine::MEngine Engine;
    Engine.PushGuiLayer(new MamontEditor::Editor());

    Engine.Init();
    
    Engine.Run();

    Engine.Cleanup();

    return 0;
}