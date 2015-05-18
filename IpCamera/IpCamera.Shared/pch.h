#pragma once

#include <list>
#include <memory>
#include <sstream>

#include <collection.h>
#include <ppltasks.h>
#include <wrl.h>
#include <robuffer.h>

//
// Asserts
//

#ifndef NDEBUG
#define NT_ASSERT(expr) if (!(expr)) { __debugbreak(); }
#else
#define NT_ASSERT(expr)
#endif

//
// Tracing
//

#include "DebuggerLogger.h"

#define Trace(format, ...) { \
    if(s_logger.IsEnabled(LogLevel::Information)) { s_logger.Log(__FUNCTION__, LogLevel::Information, format, __VA_ARGS__); } \
}
#define TraceError(format, ...) { \
    if(s_logger.IsEnabled(LogLevel::Error)) { s_logger.Log(__FUNCTION__, LogLevel::Error, format, __VA_ARGS__); } \
}

//
// Error handling
//

// Exception-based error handling
#define CHK(statement)  {HRESULT _hr = (statement); if (FAILED(_hr)) { throw ref new Platform::COMException(_hr); };}
#define CHKNULL(p)  {if ((p) == nullptr) { throw ref new Platform::NullReferenceException(L#p); };}
#define CHKOOM(p)  {if ((p) == nullptr) { throw ref new Platform::OutOfMemoryException(L#p); };}

//
// IBuffer data access
//

inline unsigned char* GetData(Windows::Storage::Streams::IBuffer^ buffer)
{
    unsigned char* bytes = nullptr;
    Microsoft::WRL::ComPtr<::Windows::Storage::Streams::IBufferByteAccess> bufferAccess;
    CHK(((IUnknown*)buffer)->QueryInterface(IID_PPV_ARGS(&bufferAccess)));
    CHK(bufferAccess->Buffer(&bytes));
    return bytes;
}
