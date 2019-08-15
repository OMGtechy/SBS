#pragma once

#ifndef SBS_ENABLE_DEBUG_ASSERTIONS
#ifndef NDEBUG
#define SBS_ENABLE_DEBUG_ASSERTIONS 1
#endif
#endif // SBS_ENABLE_DEBUG_ASSERTIONS

#if SBS_ENABLE_DEBUG_ASSERTIONS
#include <cassert>
#endif // SBS_ENABLE_DEBUG_ASSERTIONS

#ifdef WIN32
#pragma comment ( lib, "Shlwapi.lib" )
#if (_MSC_VER >= 1910)
// I heard different things about the effects of __forceinline, the best source being:
// https://devblogs.microsoft.com/cppblog/visual-studio-2017-throughput-improvements-and-advice/
// "The back end of the compiler takes __forceinline very, very seriously. It’s exempt from all
// inline budget checks (the cost of a __forceinline function doesn’t even count against the inline budget)
// and is always honored."
// The article is about VS 2017, so I assume it works as intended from that point onwards.
// See the unscoped_stack_vector constructor for why inlining is so important here.
#define FORCE_INLINE __forceinline
#else
#error "Unsupported MSVC version!"
#endif
#else
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif

#include <new>
#include <sstream>

namespace sbs {
    //! \brief  A stack allocated runtime-sized array.
    //! \note   The memory allocated by this type is not free'd until the enclosing function ends.
    //!         This is why it's an "unscoped" dynarray (it may also prompt you to read this).
    //! \note   This type is not thread safe (if you really need it to be, you'll need to manage this yourself).
    //! \tparam  T  The value type
    template <typename T>
    class unscoped_stack_vector final {
    public:
        using value_type = T;
        using size_type = std::size_t;

        //! \brief  Creates an instance of dynarray.
        //! \note   This *must* be inlined, since the scope of the alloca depends on the stack frame.
        //! \note   This doesn't follow "normal" scoping rules - its memory will be free'd when the enclosing function
        //!         ends, so be very careful of putting this inside a loop (you may get a stack overflow / worse).
        //! \param  capacity  The maximum capacity of the array. If this is 0, the behaviour is undefined.
        //!                   You should also be careful not to blow your stack!
		FORCE_INLINE
        explicit constexpr unscoped_stack_vector(const size_type max_size) noexcept
        : m_data(static_cast<value_type*>(alloca(max_size * sizeof(value_type)))),
          m_max_size(max_size),
          m_size(0) {
            #if SBS_ENABLE_DEBUG_ASSERTIONS
                assert(max_size != 0);
                assert(m_data != nullptr);

                // this makes sure the following alignment check is valid, because of the casts it uses
                static_assert(alignof(T) <= std::numeric_limits<uintptr_t>::max());
                assert(reinterpret_cast<uintptr_t>(m_data) % static_cast<uintptr_t>(alignof(T)) == 0);
            #endif // SBS_ENABLE_DEBUG_ASSERTIONS
        }

        //! \brief  Creates an instance of dynarray with an initial size.
        //! \note   See the other constructor for the rest of the documentation.
        //! \param  initial_size  The initial size to use.
		FORCE_INLINE
        explicit constexpr unscoped_stack_vector(const size_type max_size,
                                                        const size_type initial_size) noexcept(std::is_nothrow_default_constructible_v<value_type>)
        : unscoped_stack_vector(max_size)
        {
            #if SBS_ENABLE_DEBUG_ASSERTIONS
                assert(max_size >= initial_size);
            #endif // SBS_ENABLE_DEBUG_ASSERTIONS

            for(size_type i = 0; i < initial_size; ++i) {
                push_back(value_type());
            }
        }

        // non-copyable
        unscoped_stack_vector(const unscoped_stack_vector&) = delete;
        unscoped_stack_vector& operator=(const unscoped_stack_vector&) = delete;

        // non-moveable
        unscoped_stack_vector(unscoped_stack_vector&&) = delete;
        unscoped_stack_vector& operator=(unscoped_stack_vector&&) = delete;

        constexpr size_type size() const noexcept { return m_size; }
        constexpr size_type max_size() const noexcept { return m_max_size; }

        constexpr value_type& operator[](const size_type index) noexcept
        {
            #if SBS_ENABLE_DEBUG_ASSERTIONS
                assert(index < size());
            #endif // SBS_ENABLE_DEBUG_ASSERTIONS

            return m_data[index];
        }

        constexpr const value_type& operator[](const size_type index) const noexcept
        {
            #if SBS_ENABLE_DEBUG_ASSERTIONS
                assert(index < size());
            #endif // SBS_ENABLE_DEBUG_ASSERTIONS

            return m_data[index];
        }

        constexpr const value_type& at(const size_type index) const
        {
            at_bounds_check(index);
            return (*this)[index];
        }

        constexpr value_type& at(const size_type index)
        {
            at_bounds_check(index);
            return (*this)[index];
        }

        constexpr const value_type& front() const noexcept
        {
            return this->operator[][0];
        }

        constexpr value_type& front() noexcept
        {
            return (*this)[0];
        }

        constexpr const value_type& back() const noexcept
        {
            return (*this)[size() - 1];
        }

        constexpr value_type& back() noexcept
        {
            return (*this)[size() - 1];
        }

        constexpr const value_type* data() const noexcept
        {
            return m_data;
        }

        constexpr value_type* data() noexcept
        {
            return m_data;
        }

        template <typename Data>
        constexpr void push_back(Data&& data)
        {
            #if SBS_ENABLE_DEBUG_ASSERTIONS
                assert(size() < max_size());
            #endif // SBS_ENABLE_DEBUG_ASSERTIONS

            const auto targetAddress = &(m_data[m_size++]);

            if constexpr (std::is_move_constructible_v<value_type>) {
                new(targetAddress) value_type(std::forward<Data>(data));
            } else {
                new (targetAddress) value_type(data);
            }
        }

        constexpr void pop_back() noexcept(std::is_nothrow_destructible_v<value_type>)
        {
            #if SBS_ENABLE_DEBUG_ASSERTIONS
                assert(size() != 0);
            #endif // SBS_ENABLE_DEBUG_ASSERTIONS

            m_data[--m_size].~value_type();
        }

        template <typename ...Args>
        constexpr void emplace_back(Args&&... args) noexcept(noexcept(value_type(std::forward<Args>(args)...)))
        {
            push_back(value_type(std::forward<Args>(args)...));
        }

    private:
        constexpr void at_bounds_check(const size_type index) const
        {
            // TODO: likely() unlikely()?
            if(!(index < size())) {
                std::stringstream error;
                error << "Index " << index << " is not a valid index for an unscoped_stack_vector of size "
                      << size() << " (max size " << max_size() << ")";
                throw std::out_of_range(error.str());
            }
        }

        value_type* const m_data;
        const size_type m_max_size;
        size_type m_size;
    };
}
