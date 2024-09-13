#include <iostream>
#include <algorithm>
#include <string>
#include <cwctype>

int main()
{
    while (true)
    {
        std::wstring str;
        std::wcin >> str;
        std::transform(str.begin(), str.end(), str.begin(), std::towlower);
        std::wcout << str << "\n";
    }

    return EXIT_SUCCESS;
}