import xml.etree.ElementTree as ET
proj = r"C:\Users\it252195\source\repos\Dual\Dual\Dual.vcxproj"
ns = {"msbuild": "http://schemas.microsoft.com/developer/msbuild/2003"}
ET.register_namespace("", ns["msbuild"])
tree = ET.parse(proj)
root = tree.getroot()

found_packages = []
for item_group in root.findall("msbuild:ItemGroup", ns):
    for item in item_group.findall("msbuild:None", ns):
        if item.attrib.get("Include") == "packages.config":
            found_packages.append(item)
            if len(found_packages) > 1:
                item_group.remove(item)
                print("Removed a duplicate packages.config from Dual!")

if len(found_packages) > 1:
    tree.write(proj, encoding="utf-8", xml_declaration=True)
    print("Saved file Dual.")
else:
    print(f"Found {len(found_packages)} packages.config in Dual")
