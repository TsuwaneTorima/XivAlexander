#pragma once

#include <heapapi.h>
#include <memory>
#include "Utils_Win32_Closeable.h"

namespace Utils::Win32 {

	using Heap = Closeable<HANDLE, HeapDestroy>;

	template<typename T>
	class HeapAllocator {
		template<typename U>
		friend class HeapAllocator;

	public:
		typedef T value_type;

	private:
		std::shared_ptr<Heap> m_heap;

	public:
		explicit HeapAllocator(_In_ DWORD flOptions,
			_In_ SIZE_T dwInitialSize = 0,
			_In_ SIZE_T dwMaximumSize = 0)
			: m_heap(std::make_shared<Heap>(HeapCreate(flOptions, dwInitialSize, dwMaximumSize), Heap::Null,
				"HeapCreate(0x{:x}, 0x{:x}, 0x{:}", flOptions, dwInitialSize, dwMaximumSize)) {
		}

		HeapAllocator(std::shared_ptr<Heap> heap = nullptr)
			: m_heap(std::move(heap)) {
		}

		template<typename U>
		HeapAllocator(HeapAllocator<U>&& r)
			: m_heap(r.m_heap) {
			// r.m_heap does not change per Allocator specifications.
		}

		template<typename U>
		HeapAllocator<T>& operator=(HeapAllocator<U>&& r) {
			m_heap = r.m_heap;
			// r.m_heap does not change per Allocator specifications.
			return *this;
		}

		template<typename U>
		HeapAllocator(const HeapAllocator<U>& r)
			: m_heap(r.m_heap) {
		}

		template<typename U>
		HeapAllocator<T>& operator=(const HeapAllocator<U>& r) {
			m_heap = r.m_heap;
			return *this;
		}

		~HeapAllocator() = default;

		[[nodiscard]] T* allocate_nothrow(std::size_t n) noexcept {
			if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
				return nullptr;

			if (auto p = static_cast<T*>(HeapAlloc(GetCurrentAllocatorHeap(), 0, n * sizeof(T))))
				return p;

			return nullptr;
		}

		[[nodiscard]] T* allocate(std::size_t n) {
			if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
				throw std::bad_array_new_length();

			if (auto p = static_cast<T*>(HeapAlloc(GetCurrentAllocatorHeap(), 0, n * sizeof(T))))
				return p;

			throw std::bad_alloc();
		}

		void deallocate(T* p, std::size_t) noexcept {
			HeapFree(GetCurrentAllocatorHeap(), 0, p);
		}

		[[nodiscard]] auto GetCurrentAllocatorHeap() const {
			if (m_heap)
				return static_cast<HANDLE>(*m_heap);
			if (g_hDefaultHeap)
				return g_hDefaultHeap;
			return GetProcessHeap();
		}
	};

	template<typename T, typename U>
	bool operator==(const HeapAllocator<T>& l, const HeapAllocator<U>& r) {
		return l.m_heap == r.m_heap;
	}

	template<typename T, typename U>
	bool operator!=(const HeapAllocator<T>& l, const HeapAllocator<U>& r) {
		return l.m_heap != r.m_heap;
	}
}
