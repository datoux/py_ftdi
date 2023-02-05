/**
 * Copyright (C) 2023 Daniel Turecek
 *
 * @file      buffer.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2023-02-05
 *
 */
#ifndef BUFFER_H
#define BUFFER_H
#include <memory>
#include <memory.h>
#include <cassert>

template <class T> class Buffer
{
public:
    Buffer(size_t size = 0, bool shrinkable = false)
        : mBuff(0)
        , mSize(size)
        , mAllocatedSize(size)
        , mShrinkable(shrinkable)
    {
        if (size)
            mBuff = new T[size];
    }

    Buffer(const Buffer<T> &b)
        : mBuff(0)
        , mSize(b.mSize)
        , mAllocatedSize(b.mSize)
        , mShrinkable(b.mShrinkable)
    {
        if (mSize) {
            mBuff = new T[mSize];
            memcpy(mBuff, b.mBuff, b.byteSize());
        }
    }

    ~Buffer(){
        delete[] mBuff;
    }

    Buffer<T> & operator=(const Buffer<T> &b) {
        if (this != &b) {
            if (mSize != b.mSize)
                reinit(b.mSize);
            memcpy(mBuff, b.mBuff, byteSize());
        }
        return *this;
    }

    bool operator==(const Buffer<T> &b) {
        return mSize == b.size() && memcmp(mBuff, b.data(), byteSize()) == 0;
    }

    void setVal(T val) {
        if (mBuff)
            for (size_t i = 0; i < mSize; i++)
                mBuff[i] = val;
    }

    void zero() {
        if (mBuff)
            memset(mBuff, 0, mSize*sizeof(T));
    }

    void reinit(size_t size) {
        if (size == mSize) return;
        if (size > mAllocatedSize || mShrinkable) {
            if (mBuff)
                delete[] mBuff;
            mAllocatedSize = mSize = 0; // in case new fails
            mBuff = new T[size];
            mAllocatedSize = mSize = size;
        }else
            mSize = size;
    }

    void reinit(size_t size, T val) {
        reinit(size);
        setVal(val);
    }

    template<typename U> void assignData(U *data, size_t size) {
        if (size != mSize)
            reinit(size);
        for (size_t i = 0; i < size; i++)
            mBuff[i] = (T)data[i];
    }

    // exchange all inner data between two buffers
    void exchangeBuffers(Buffer<T> &b)
    {
        T* tmpbuff = b.mBuff; b.mBuff = mBuff; mBuff = tmpbuff;
        size_t tmpsize = b.mSize; b.mSize = mSize; mSize = tmpsize;
        size_t tmpalloc = b.mAllocatedSize; b.mAllocatedSize = mAllocatedSize; mAllocatedSize = tmpalloc;
        bool tmpshrink = b.mShrinkable; b.mShrinkable = mShrinkable; mShrinkable = tmpshrink;
    }

    void clear() {
        delete[] mBuff;
        mBuff = 0;
        mAllocatedSize = mSize = 0;
    }

public:
    operator T*()             { return mBuff; }
    T* data()                 { return mBuff; }
    const T* data() const     { return mBuff; }
    size_t size() const       { return mSize; }
    size_t byteSize() const   { return mSize*sizeof(T); }
    T& get(size_t i) const    { return mBuff[i]; }
    void set(size_t i, T val) { mBuff[i] = val; }
    T& last()                 { return mBuff[mSize - 1]; }
    bool empty() const        { return mSize == 0; }
    void replaceInnerBuff(T* buff) { if (mBuff) delete[] mBuff; mBuff = buff; }

private:
    T* mBuff;
    size_t mSize;
    size_t mAllocatedSize;
    bool mShrinkable;
};

#endif /* end of include guard: BUFFER_H */




