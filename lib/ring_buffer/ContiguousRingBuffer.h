// From https://gist.githubusercontent.com/jahnf/ba6b352a9d2b508200442a4ec40ff57c/raw/eab487f11d388341402f91d666934dcc1bd4552e/contiguous_ring_buffer.h
#pragma once

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace common
{
  // -----------------------------------------------------------------------------------------------
  /// ContiguousRingbuffer always guarantees access to the buffer as a
  /// contiguous array in memory.
  ///
  /// This buffer will use a std::array of type T with the size 2*N-1.
  template <typename T, std::size_t N>
  class ContiguousRingBuffer
  {
  public:
    using buf_type = std::array<T, N*2-1>;
    using typename buf_type::const_iterator;
    using typename buf_type::iterator;
    using typename buf_type::value_type;
    using typename buf_type::reference;
    using typename buf_type::size_type;

	// inline constexpr bool is_trivial_v = std::is_trivial<T>::value;
    static_assert(std::is_trivial<T>::value, "T must be trivial.");

    // -------------------------------------------------------------------------
    // ContiguousRingBuffer() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
	ContiguousRingBuffer() = default;
    ContiguousRingBuffer(ContiguousRingBuffer&&) = default;
    ContiguousRingBuffer(ContiguousRingBuffer const&) = default;

    /// Iterator pair constructor.
    template <typename It
      , typename from_value_type = typename std::iterator_traits<It>::value_type
      , typename to_value_type = typename std::iterator_traits<iterator>::value_type
    //   , typename = std::enable_if<std::is_nothrow_convertible_v<from_value_type, to_value_type>>
      >
    ContiguousRingBuffer(It beg, It end)
    {
      if (static_cast<std::size_t>(std::distance(beg, end)) > N) {
        std::copy(std::prev(end, N), end, m_data.begin());
        m_size = N;
      }
      else {
        auto const e = std::copy(beg, end, m_data.begin());
        m_size = std::distance(m_data.begin(), e);
      }
    }

    /// Initialize from any other ContinguousBuffer<S, M>
    template <typename S, std::size_t M>
    explicit ContiguousRingBuffer(ContiguousRingBuffer<S, M> const& o)
      : ContiguousRingBuffer(o.cbegin(), o.cend())
    {}

    /// Initializer list constructor.
    ContiguousRingBuffer(std::initializer_list<T> init)
      : ContiguousRingBuffer(init.begin(), init.end())
    {}

    ContiguousRingBuffer& operator=(ContiguousRingBuffer&&) = default;
    ContiguousRingBuffer& operator=(ContiguousRingBuffer const&) = default;

    // -------------------------------------------------------------------------
    constexpr iterator begin() noexcept { return m_data.begin() + m_start; }
    constexpr iterator end() noexcept { return begin() + m_size; }
    constexpr iterator begin() const noexcept { return m_data.begin() + m_start; }
    constexpr iterator end() const noexcept { return begin() + m_size; }
    constexpr const_iterator cbegin() const noexcept { return m_data.begin() + m_start; }
    constexpr const_iterator cend() const noexcept { return cbegin() + m_size; }

    // -------------------------------------------------------------------------
    T const& operator[](std::size_t pos) const { return *(cbegin() + pos); }
    T& operator[](std::size_t pos) { return *(begin() + pos); }

    // -------------------------------------------------------------------------
    constexpr std::size_t size() const noexcept { return m_size; }
    constexpr std::size_t max_size() const noexcept { return N; }
    constexpr std::size_t capacity() const noexcept { return N; }

    // -------------------------------------------------------------------------
    T* data() { return &(*begin()); }
    T const* data() const { return &(*begin()); }

    // -------------------------------------------------------------------------
    void clear() noexcept { m_start = m_size = 0; }
    constexpr bool is_empty() const noexcept { return m_size == 0; }
    constexpr bool empty() const noexcept { return m_size == 0; }

    // -------------------------------------------------------------------------
    inline void pop_front()
    {
      if (m_size > 0) {
        ++m_start;
        if (--m_size == 0) {
          clear();
        }
      }
    }

    // -------------------------------------------------------------------------
    inline void pop() { pop_front(); }

    // -------------------------------------------------------------------------
    void pop_front(std::size_t num_elements)
    {
      if (num_elements < m_size) {
        m_start += num_elements;
        m_size -= num_elements;
      }
      else {
        clear();
      }
    }

    // -------------------------------------------------------------------------
    void pop_back()
    {
      if (m_size > 0) {
        if (--m_size == 0) {
          clear();
        }
      }
    }

    // -------------------------------------------------------------------------
    void pop_back(std::size_t num_elements)
    {
      if (num_elements < m_size) {
        m_size -= num_elements;
      }
      else {
        clear();
      }
    }

    // -------------------------------------------------------------------------
    template <typename U
    //   , typename = std::enable_if_t<
    //       std::is_nothrow_convertible_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>>
      >
    void push_back(U&& value)
    {
      // Enough space, just append and adjust size and start
      if (end() != m_data.end())
      {
        *(end()) = std::forward<U>(value);
        if (m_size < N) {
          ++m_size;
        }
        else {
          ++m_start;
        }
      }
      // Not enough space, copy to beginning of array and append
      else
      {
        auto e = std::copy(
          (size() < N) ? begin() : (begin() + 1)
          , m_data.end(), m_data.begin());

        m_start = 0;
        *(e++) = std::forward<U>(value);
        m_size = std::distance(m_data.begin(), e);
      }
    }

    // -------------------------------------------------------------------------
    template <typename U
    //   , typename = std::enable_if_t<
    //       std::is_nothrow_convertible_v<std::remove_cvref_t<U>, std::remove_cvref_t<T>>>
      >
    inline void push(U&& value) { push_back(std::forward<U>(value)); }

    // -------------------------------------------------------------------------
    template <typename It
      , typename from_value_type = typename std::iterator_traits<It>::value_type
      , typename to_value_type = typename std::iterator_traits<iterator>::value_type
    //   , typename = std::enable_if<std::is_nothrow_convertible_v<from_value_type, to_value_type>>
      >
    void push_back(It beg_, It end_)
    {
      // If inserted size is equal or larger than N, just copy the last N
      // elements of the range and reset m_start and m_size
      std::size_t const inputLen = std::distance(beg_, end_);

	// c++17 workaround
	std::size_t const space_right = std::distance(end(), m_data.end());

      if (inputLen >= N)
      {
        std::copy(std::prev(end_, N), end_, m_data.begin());
        m_start = 0;
        m_size = N;
        return;
      }
      // Check if there is enough space to copy/append the elements at the end
      else if (
               inputLen <= space_right)
      {
        std::copy(beg_, end_, end());
        if (m_size + inputLen > N) {
          m_start += (m_size + inputLen) - N;
          m_size = N;
        }
        else
        {
          m_size += inputLen;
        }
        return;
      }
      else
      {
        auto const combinedLen = (inputLen + m_size);
        // copy current buffer (with possible offset) content to the internal buffer start...
        auto const copy_beg = (combinedLen > N) ? (begin() + (combinedLen - N)) : begin();
        auto const copy_end = std::copy(copy_beg, end(), m_data.begin());
        m_start = 0;

        // ... and append new elements
        auto const append_end = std::copy(beg_, end_, copy_end);
        m_size = std::distance(m_data.begin(), append_end);
      }
    }

    // -------------------------------------------------------------------------
    void push_back(std::initializer_list<T> init_list)
    {
      push_back(init_list.begin(), init_list.end());
    }

  private:
    buf_type m_data;
    size_t m_start = 0;
    size_t m_size = 0;
  };

} // end namespace common