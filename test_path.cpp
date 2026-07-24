#include <iostream>
#include <string>
#include <filesystem>

std::wstring MakeRelativePath(const std::wstring& absolutePath)
{
    std::wstring result = absolutePath;

    size_t pos = result.find(L"Resources");
    if (pos == std::wstring::npos) 
    {
        pos = result.find(L"resources");
    }

    if (pos != std::wstring::npos)
    {
        result = result.substr(pos);
    }
    else
    {
        try
        {
            std::filesystem::path fullPath(absolutePath);
            std::filesystem::path currentDir = std::filesystem::current_path();
            result = std::filesystem::relative(fullPath, currentDir).wstring();
        }
        catch (...)
        {
            result = absolutePath;
        }
    }

    for (wchar_t& c : result)
    {
        if (c == L'\\') c = L'/';
    }

    return result;
}

int main() {
    std::wcout << MakeRelativePath(L"C:\\Users\\it252195\\Documents\\Visual Studio 18\\Dual\\Dual\\Resources\\Models\\knight\\idle.sdkmesh_anim") << std::endl;
    return 0;
}
