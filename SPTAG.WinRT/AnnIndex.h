#pragma once

#include "AnnIndex.g.h"
#include "inc/Core/VectorIndex.h"
#include "inc/Core/SearchQuery.h"
#include <array>
#include <vector>

#include <winrt/Windows.Storage.h>
namespace sptag = ::SPTAG;
namespace winrt::SPTAG::implementation
{
  constexpr size_t EmbeddingSize = 1024;
  float* CopyArray(const winrt::array_view<const float>& data) {
    if (data.size() != EmbeddingSize) throw winrt::hresult_invalid_argument{}; 
    auto copy = new float[EmbeddingSize];
    for (auto i = 0u; i < EmbeddingSize; i++) copy[i] = data[i];
    return copy;
  }

  sptag::ByteArray GetByteArray(const winrt::array_view<const float>& data) {
    auto copy = CopyArray(data);
    auto byteArray = sptag::ByteArray(reinterpret_cast<byte*>(copy), data.size() * sizeof(float) / sizeof(byte), true);
    return byteArray;
  }

  sptag::ByteArray GetByteArray(const winrt::array_view<const uint8_t>& data) {
    auto vec = new uint8_t[data.size()];
    for (auto i = 0u; i < data.size(); i++) vec[i] = data[i];
    auto byteArray = sptag::ByteArray(vec, data.size(), true);
    return byteArray;
  }

  sptag::ByteArray GetByteArray(const winrt::hstring& data) {
    int const size = WINRT_IMPL_WideCharToMultiByte(65001 /*CP_UTF8*/, 0, data.data(), static_cast<int32_t>(data.size()), nullptr, 0, nullptr, nullptr);

    if (size == 0) {
      return{};
    }
    auto result = new uint8_t[size + 1];
    static_assert(sizeof(char) == sizeof(uint8_t));
    WINRT_IMPL_WideCharToMultiByte(65001 /*CP_UTF8*/, 0, data.data(), static_cast<int32_t>(data.size()), reinterpret_cast<char*>(result), size, nullptr, nullptr);
    auto byteArray = sptag::ByteArray(result, size, true);
    return byteArray;
  }

  struct AnnIndex : AnnIndexT<AnnIndex>
  {
    sptag::DimensionType m_dimension{ 1024 };
    sptag::VectorValueType m_inputValueType{ sptag::VectorValueType::Float };

    AnnIndex() {
      sptag::g_pLogger.reset(new sptag::Helper::SimpleLogger(sptag::Helper::LogLevel::LL_Empty));
      m_index = sptag::VectorIndex::CreateInstance(sptag::IndexAlgoType::BKT, sptag::GetEnumValueType<float>());
      
    }


    template<typename T> // works for hstring and winrt::array_view<const uint8>
    void _AddWithMetadataImpl(winrt::array_view<const float> p_data, T metadata) {
      int p_num{ 1 };
      auto byteArray = GetByteArray(p_data);
      std::shared_ptr<sptag::VectorSet> vectors(new sptag::BasicVectorSet(byteArray,
        m_inputValueType,
        static_cast<sptag::DimensionType>(m_dimension),
        static_cast<sptag::SizeType>(p_num)));


      sptag::ByteArray p_meta = GetByteArray(metadata);
      bool p_withMetaIndex{ true };
      bool p_normalized{ true };

      std::uint64_t* offsets = new std::uint64_t[p_num + 1]{ 0 };
      if (!sptag::MetadataSet::GetMetadataOffsets(p_meta.Data(), p_meta.Length(), offsets, p_num + 1, '\n')) throw winrt::hresult_invalid_argument{};
      std::shared_ptr<sptag::MetadataSet> meta(new sptag::MemMetadataSet(p_meta, sptag::ByteArray((std::uint8_t*)offsets, (p_num + 1) * sizeof(std::uint64_t), true), (sptag::SizeType)p_num));
      if (sptag::ErrorCode::Success != m_index->AddIndex(vectors, meta, p_withMetaIndex, p_normalized)) {
        throw winrt::hresult_error(E_UNEXPECTED);
      }
    }

    void AddWithMetadata(winrt::array_view<const float> p_data, winrt::hstring metadata) {
      _AddWithMetadataImpl(p_data, metadata);
    }


    void Save(winrt::Windows::Storage::StorageFile file){
      auto path = file.Path();
      if (sptag::ErrorCode::Success != m_index->SaveIndexToFile(winrt::to_string(path))) {
        throw winrt::hresult_error{};
      }
    }

    winrt::Windows::Foundation::Collections::IVector<SPTAG::SearchResult> Search(winrt::array_view<const float> p_data, uint8_t p_resultNum) const {
      auto vec = std::vector<SPTAG::SearchResult>{};
      auto results = std::make_shared<sptag::QueryResult>(p_data.data(), p_resultNum, true);

      if (nullptr != m_index) {
        m_index->SearchIndex(*results);
      }
      for (auto& r : *results) {
        auto ba = r.Meta;

        int const size = WINRT_IMPL_MultiByteToWideChar(65001 /*CP_UTF8*/, 0, reinterpret_cast<char*>(ba.Data()), static_cast<int32_t>(ba.Length()), nullptr, 0);

        if (size == 0) {
          return{};
        }

        impl::hstring_builder result(size);
        WINRT_VERIFY_(size, WINRT_IMPL_MultiByteToWideChar(65001 /*CP_UTF8*/, 0, reinterpret_cast<char*>(ba.Data()), static_cast<int32_t>(ba.Length()), result.data(), size));

        auto meta = result.to_hstring();
        vec.push_back(SPTAG::SearchResult{ meta, r.Dist });
      }
      return winrt::single_threaded_vector<SPTAG::SearchResult>(std::move(vec));
    }

    std::shared_ptr<sptag::VectorIndex> m_index;
  };
}

namespace winrt::SPTAG::factory_implementation
{
    struct AnnIndex : AnnIndexT<AnnIndex, implementation::AnnIndex>
    {
    };
}
