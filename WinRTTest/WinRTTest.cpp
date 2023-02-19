#include <iostream>
#include <winrt/SPTAG.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
int main()
{
    SPTAG::AnnIndex idx;
    using embedding_t = std::array<float, 1024>;
    idx.AddWithMetadata(embedding_t{ 1,0 }, L"first one");
    idx.AddWithMetadata(embedding_t{ 0,1,0 }, L"second one");
    auto res = idx.Search(embedding_t{ 0.f, 0.99f, 0.01f }, 2);
    for (const auto& r : res) {
      std::wcout << r.Metadata << L" -- " << r.Distance  << L"\n";
    }

}

