#pragma once

#include <core/memory/allocator_utils.h>
#include <core/memory/allocator_types.h>
#include <core/memory/iallocator.h>
#include <core/memory/heap_allocator.h>
#include <core/memory/linear_allocator.h>
#include <core/memory/stack_allocator.h>
#include <core/memory/pool_allocator.h>

// Alloc
#define IMP_ALLOC(allocator_, bytes_, alignment_, tag_) \
    (allocator_).alloc((bytes_), (alignment_), (tag_))

#define IMP_ALLOC_DEFAULT(allocator_, bytes_) \
    (allocator_).alloc((bytes_))

// Free
#define IMP_FREE(allocator_, ptr_, bytes_, tag_) \
    (allocator_).free((ptr_), (bytes_), (tag_))

// New
#define IMP_NEW(Type_, allocator_, tag_, ...) \
    ::imp::memory::allocNew<Type_>((allocator_), (tag_) __VA_OPT__(,) __VA_ARGS__)

// Delete
#define IMP_DELETE(Type_, allocator_, ptr_, tag_) \
    ::imp::memory::allocDelete<Type_>((allocator_), (ptr_), (tag_))

// Scratch
#define IMP_SCRATCH(allocator_, Type_, count_) \
    static_cast<Type_*>((allocator_).alloc( \
        sizeof(Type_) * (count_), alignof(Type_), ::imp::memory::MemTag::Scratch))