#include "Core/Engine.h"
#include "Editor.h"


int main(int argc, char** argv) 
{
    MamontEngine::MEngine Engine;
    Engine.Init();
    Engine.PushGuiLayer(new MamontEditor::Editor());
    
    Engine.Run();

    Engine.Cleanup();

    return 0;
}