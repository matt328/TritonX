#pragma once

namespace TX::Graphics {
   inline void ThrowIfFailed(HRESULT hr) {
      if (FAILED(hr)) {
         _com_error err(hr);
         OutputDebugString(err.ErrorMessage());

         throw std::exception();
      }
   }
}
