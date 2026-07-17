import os
import shutil
import xml.etree.ElementTree as ET

engine_dir = r"C:\Users\it252195\source\repos\Dual\HEIN_Engine"
dual_dir = r"C:\Users\it252195\source\repos\Dual\Dual"

files_to_move = [
    "Factory/ActorFactory.h",
    "Factory/ActorFactory.cpp",
    "States/CombatStates.h",
    "States/CombatStates.cpp",
    "States/ICombatState.h",
    "BlackBoard/CombatBlackBoard.h",
    "Components/CombatStateMachineComponent.h",
    "Components/CombatStateMachineComponent.cpp",
    "Components/DamageDealerComponent.h",
    "Components/DamageDealerComponent.cpp",
    "Components/HealthComponent.h",
    "Components/HealthComponent.cpp",
    "Components/PlayerInputComponent.h",
    "Components/PlayerInputComponent.cpp",
    "BehaviourTree/BTAttackNode.h",
    "BehaviourTree/BTAttackNode.cpp",
    "BehaviourTree/BTChaseNode.h",
    "BehaviourTree/BTChaseNode.cpp",
    "BehaviourTree/BTCheckDistance.h",
    "BehaviourTree/BTCheckDistance.cpp",
    "BehaviourTree/BTDodgeNode.h",
    "BehaviourTree/BTDodgeNode.cpp",
    "BehaviourTree/BTStrafeNode.h",
    "BehaviourTree/BTStrafeNode.cpp"
]

ns = {"msbuild": "http://schemas.microsoft.com/developer/msbuild/2003"}
ET.register_namespace("", ns["msbuild"])

def move_files():
    for f in files_to_move:
        src = os.path.join(engine_dir, f.replace("/", "\\"))
        dst = os.path.join(dual_dir, f.replace("/", "\\"))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        if os.path.exists(src):
            shutil.move(src, dst)
            print(f"Moved {f}")

def remove_from_proj(proj_path, file_list):
    tree = ET.parse(proj_path)
    root = tree.getroot()
    changed = False
    for item_group in root.findall("msbuild:ItemGroup", ns):
        items_to_remove = []
        for item in item_group:
            if item.tag.endswith("ClInclude") or item.tag.endswith("ClCompile"):
                inc = item.attrib.get("Include", "").replace("\\", "/")
                if inc in file_list:
                    items_to_remove.append(item)
        for item in items_to_remove:
            item_group.remove(item)
            changed = True
    if changed:
        tree.write(proj_path, encoding="utf-8", xml_declaration=True)
        print(f"Updated {proj_path}")

def add_to_proj(proj_path, file_list):
    tree = ET.parse(proj_path)
    root = tree.getroot()
    
    # find where to insert
    target_import = None
    for child in root:
        if child.tag == "{http://schemas.microsoft.com/developer/msbuild/2003}Import" and "Microsoft.Cpp.targets" in child.attrib.get("Project", ""):
            target_import = child
            break
            
    group_h = ET.Element("ItemGroup")
    group_cpp = ET.Element("ItemGroup")
    
    for f in file_list:
        f_win = f.replace("/", "\\")
        if f.endswith(".h"):
            ET.SubElement(group_h, "ClInclude", Include=f_win)
        elif f.endswith(".cpp"):
            el = ET.SubElement(group_cpp, "ClCompile", Include=f_win)
            
    # Insert before targets import
    if target_import is not None:
        idx = list(root).index(target_import)
        root.insert(idx, group_cpp)
        root.insert(idx, group_h)
    else:
        root.append(group_cpp)
        root.append(group_h)
        
    tree.write(proj_path, encoding="utf-8", xml_declaration=True)
    print(f"Added files to {proj_path}")

print("Starting...")
move_files()
remove_from_proj(os.path.join(engine_dir, "HEIN_Engine.vcxproj"), files_to_move)
remove_from_proj(os.path.join(engine_dir, "HEIN_Engine.vcxproj.filters"), files_to_move)

add_to_proj(os.path.join(dual_dir, "Dual.vcxproj"), files_to_move)
add_to_proj(os.path.join(dual_dir, "Dual.vcxproj.filters"), files_to_move) # filters don't technically need strict ordering but it's fine

