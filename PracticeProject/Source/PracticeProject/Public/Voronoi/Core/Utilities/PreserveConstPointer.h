// By Polyakov Pavel

#pragma once

template <typename T>
class PRACTICEPROJECT_API TPreserveConstPointer
{
    T *Data;

public:
    FORCEINLINE TPreserveConstPointer(T *InData = nullptr) : Data(InData) {}
    FORCEINLINE TPreserveConstPointer& operator=(T *InData) { Data = InData; return *this; }

    FORCEINLINE TPreserveConstPointer(TPreserveConstPointer& Other) : Data(Other.Data) {}
    FORCEINLINE TPreserveConstPointer& operator=(TPreserveConstPointer& Other) { Data = Other.Data; return *this; }

    FORCEINLINE operator T*() { return Data; }
    FORCEINLINE operator const T*() const { return Data; }

    FORCEINLINE T* operator->() { return Data; }
    FORCEINLINE const T* operator->() const { return Data; }
};
