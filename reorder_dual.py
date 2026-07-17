import xml.etree.ElementTree as ET
import os

proj_path = r"C:\Users\it252195\source\repos\Dual\Dual\Dual.vcxproj"
ns = {"msbuild": "http://schemas.microsoft.com/developer/msbuild/2003"}
ET.register_namespace("", ns["msbuild"])

tree = ET.parse(proj_path)
root = tree.getroot()

# Find the Microsoft.Cpp.targets import
target_import = None
extension_targets = None
for child in root:
    if child.tag == "{http://schemas.microsoft.com/developer/msbuild/2003}Import":
        if "Microsoft.Cpp.targets" in child.attrib.get("Project", ""):
            target_import = child
    elif child.tag == "{http://schemas.microsoft.com/developer/msbuild/2003}ImportGroup" and child.attrib.get("Label") == "ExtensionTargets":
        extension_targets = child

if target_import is not None:
    # Remove them
    root.remove(target_import)
    if extension_targets is not None:
        root.remove(extension_targets)
    
    # And re-add them at the very end
    root.append(target_import)
    if extension_targets is not None:
        root.append(extension_targets)

tree.write(proj_path, encoding="utf-8", xml_declaration=True)
print("Reordered Dual vcxproj successfully!")
