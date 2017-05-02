// By Polyakov Pavel

#pragma once

template <typename T>
class PRACTICEPROJECT_API TPreserveConstUniquePtr
{
    TUniquePtr<T> Data;

    TPreserveConstUniquePtr(const TPreserveConstUniquePtr& Other) = delete;
    TPreserveConstUniquePtr& operator=(const TPreserveConstUniquePtr& Other) = delete;

public:
    FORCEINLINE TPreserveConstUniquePtr() : Data(nullptr) {}
	FORCEINLINE TPreserveConstUniquePtr(decltype(nullptr)) : Data(nullptr) {}
    FORCEINLINE TPreserveConstUniquePtr(TPreserveConstUniquePtr &&Other) { Swap(Data, Other.Data); }

    FORCEINLINE TPreserveConstUniquePtr(TUniquePtr<T> &&InData) : Data(MoveTemp(InData)) {}
    FORCEINLINE TPreserveConstUniquePtr& operator=(TUniquePtr<T> &&InData) { Swap(Data, InData); return *this; }

    FORCEINLINE operator T*() { return Data.Get(); }
    FORCEINLINE operator const T*() const { return Data.Get(); }

    FORCEINLINE T* operator->() { return Data.Get(); }
    FORCEINLINE const T* operator->() const { return Data.Get(); }
};
