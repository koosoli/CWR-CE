#include <Poseidon/UI/Editor/EditorModule.hpp>
#include <Poseidon/UI/GameModule.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <functional>

namespace Poseidon
{

void EditorModule::Register()
{
    GameModuleRegistry::Register({GameModuleId::Editor, "Editor", IDC_MAIN_EDITOR, CreateDisplayEditor});
}

} // namespace Poseidon
