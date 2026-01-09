#ifndef SPARSE_SET
#define SPARSE_SET

// https://github.com/MauroDeryckere/SparseSet

#include <vector>

#include <type_traits>
#include <limits>
#include <utility>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <numeric>

// memocopy gcc
#include <cstring>

#include "InternalAssert.h"

namespace Mau
{
	namespace Impl
	{
		template<typename T>
		concept KeyType = std::is_integral_v<T> && std::is_unsigned_v<T>
						&& std::is_trivially_copyable_v<T> && std::is_nothrow_swappable_v<T>  //erase && swap function
						&& std::equality_comparable<T>;

		template<typename T>
		concept MoveAssignmentVal = std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>;

		//We only want to move construct when we arent able to move assign (E.g classes with const member variables)
		template<typename T>
		concept MoveConstructVal = !(std::is_nothrow_move_assignable_v<T> || std::is_move_assignable_v<T>) 
								&& std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>;

		template<typename T>
		concept ValType =  (std::is_trivially_copyable_v<T> || MoveAssignmentVal<T> || MoveConstructVal<T>);

		template <typename C, typename T>
		concept Compare = requires(C comp, T a, T b) 
		{
			{ comp(a, b) } -> std::convertible_to<bool>;
		};

	}

	template<Impl::KeyType KeyType>
	class sparse_set_out_of_range : public std::out_of_range
	{
	public:
		sparse_set_out_of_range(std::string_view message, KeyType element) :
			std::out_of_range{ message.data() },
			m_Element{ element }
		{ }

		~sparse_set_out_of_range() = default;

		[[nodiscard]] KeyType element() const noexcept
		{
			return m_Element;
		}

	private:
		const KeyType m_Element;
	};

	template<Impl::ValType Val, Impl::KeyType KeyType = uint32_t>
	class sparse_set final
	{
	public:
		sparse_set() noexcept = default;

		sparse_set(std::initializer_list<std::pair<KeyType, Val&&>> initList, KeyType reserveSize = 0) noexcept
		{
			sparse_reserve(static_cast<KeyType>(initList.size()));
			reserve(reserveSize);

			for (auto&& pair : initList)
			{
				emplace(pair.first, std::forward<Val>(pair.second));
			}
		}

		sparse_set(KeyType sparseSize, KeyType reserveSize = 0) noexcept :
			m_SparseArr(sparseSize, INVALID_INDEX)
		{ 
			reserve(reserveSize);
		}

		~sparse_set() noexcept = default;

		sparse_set(const sparse_set& other) noexcept :
			m_SparseArr{ other.m_SparseArr },
			m_PackedValArr{ other.m_PackedValArr },
			m_DenseArr{ other.m_DenseArr }
		{ }

		sparse_set& operator=(const sparse_set& other) noexcept
		{
			m_SparseArr = other.m_SparseArr;
			m_PackedValArr = other.m_PackedValArr;
			m_DenseArr = other.m_DenseArr;

			return *this;
		}

		sparse_set(sparse_set&& other) noexcept :
			m_SparseArr{ std::move(other.m_SparseArr) },
			m_PackedValArr{ std::move(other.m_PackedValArr) },
			m_DenseArr{ std::move(other.m_DenseArr) }
		{ }

		sparse_set& operator=(sparse_set&& other) noexcept 
		{
			m_SparseArr = std::move(other.m_SparseArr);
			m_PackedValArr = std::move(other.m_PackedValArr);
			m_DenseArr = std::move(other.m_DenseArr);

			return *this;
		}

	public:
		using key_type = KeyType;
		using dense_type = KeyType;
		using value_type = Val;

		using iterator = typename std::vector<Val>::iterator;
		using const_iterator = typename std::vector<Val>::const_iterator;

		using reverse_iterator = typename std::vector<Val>::reverse_iterator;
		using const_reserve_iterator = typename std::vector<Val>::const_reverse_iterator;

		iterator begin() noexcept { return m_PackedValArr.begin(); }
		iterator end() noexcept { return m_PackedValArr.end(); }
		const_iterator begin() const noexcept { return m_PackedValArr.begin(); }
		const_iterator end() const noexcept { return m_PackedValArr.end(); }

		reverse_iterator rbegin() noexcept { return m_PackedValArr.rbegin(); }
		reverse_iterator rend() noexcept { return m_PackedValArr.rend(); }
		const_reserve_iterator rbegin() const noexcept { return m_PackedValArr.rbegin(); }
		const_reserve_iterator rend() const noexcept { return m_PackedValArr.rend(); }

		const_iterator cbegin() const noexcept { return m_PackedValArr.cbegin(); }
		const_iterator cend() const noexcept { return m_PackedValArr.cend(); }
		const_reserve_iterator crbegin() const noexcept { return m_PackedValArr.crbegin(); }
		const_reserve_iterator crend() const noexcept { return m_PackedValArr.crend(); }

	public:
		void swap(sparse_set& other) noexcept
		{
			std::swap(m_SparseArr, other.m_SparseArr);
			std::swap(m_DenseArr, other.m_DenseArr);
			std::swap(m_PackedValArr, other.m_PackedValArr);
		}

		//Should not swap elements that are not in the set, use try_swap if this is a concern
		void swap_elements(KeyType el1, KeyType el2) noexcept
		{
			ASSERT(el1 != el2, "Should not try swap element with itself!");
			ASSERT(contains(el1) && contains(el2), "Set must contain elements!");
			std::swap(m_DenseArr[m_SparseArr[el1]], m_DenseArr[m_SparseArr[el2]]);
		}
		bool try_swap_elements(KeyType el1, KeyType el2) noexcept
		{
			return (contains(el1) && contains(el2) && (swap_elements(el1, el2), true));
		}
		//Should not swap elements that are not in the set, must use valid iterators
		void swap_elements(const_iterator el1, const_iterator el2) noexcept
		{
			std::swap(m_DenseArr[val_index(el1)], m_DenseArr[val_index(el2)]);
		}

	public:
		[[nodiscard]] size_t size() const noexcept { return m_DenseArr.size(); }
		[[nodiscard]] size_t sparse_size() const noexcept { return m_SparseArr.size(); }

		[[nodiscard]] static constexpr KeyType max_sparse_size() noexcept
		{
			return INVALID_INDEX - 1;
		}

		void resize(KeyType newSize, KeyType reserveSize = 0) noexcept
		{
			ASSERT(newSize > m_SparseArr.size(), "");

			m_SparseArr.resize(newSize, INVALID_INDEX);
			m_DenseArr.reserve(reserveSize);
			m_PackedValArr.reserve(reserveSize);
		}

		void shrink_to_fit() noexcept
		{
			m_SparseArr.shrink_to_fit();
			m_DenseArr.shrink_to_fit();
			m_PackedValArr.shrink_to_fit();
		}

		void sparse_reserve(KeyType newCap) noexcept
		{
			m_SparseArr.reserve(newCap);
		}

		void reserve(KeyType newCap) noexcept
		{
			m_DenseArr.reserve(newCap);
			m_PackedValArr.reserve(newCap);
		}

		[[nodiscard]] bool empty() const noexcept { return m_DenseArr.empty(); }

		void clear() noexcept
		{
			m_DenseArr.clear();
			m_PackedValArr.clear();
			m_SparseArr.clear();
		}

		const std::vector<KeyType>& sparse() const noexcept { return m_SparseArr; }
		const std::vector<KeyType>& dense() const noexcept { return m_DenseArr; }
		const std::vector<Val>& data() const noexcept { return m_PackedValArr; }

	public:
		[[nodiscard]] bool contains(KeyType element) const noexcept 
		{ 
			ASSERT(element != INVALID_INDEX, "Element must be a valid index!");
			return element < m_SparseArr.size() && m_SparseArr[element] != INVALID_INDEX; 
		}

		//Iterator must be in bounds to get a valid value
		template <typename IteratorType>
		[[nodiscard]] KeyType sparse_index(IteratorType it) const noexcept
		{
			ASSERT(contains(m_DenseArr[val_index(it)]), "");
			return m_DenseArr[val_index(it)];
		}
		
		//Element must exist to get a valid value
		Val& operator[](KeyType element) noexcept
		{
			ASSERT(contains(element), "Element not in set!");
			return m_PackedValArr[m_SparseArr[element]];
		}
		Val const& operator[](KeyType element) const noexcept
		{
			ASSERT(contains(element), "Element not in set!");
			return m_PackedValArr[m_SparseArr[element]];
		}

		//Random access with bounds checking (similar to std::vector:::at())
		Val const& at(KeyType element)
		{
			if (contains(element))
			{
				return m_PackedValArr[m_SparseArr[element]];
			}
			throw sparse_set_out_of_range( "Element not found in sparse_set", element );
		}
		Val const& at(KeyType element) const
		{
			if (contains(element))
			{
				return m_PackedValArr[m_SparseArr[element]];
			}
			throw sparse_set_out_of_range( "Element not found in sparse_set", element );
		}

		const_iterator find(KeyType key) const noexcept
		{
			if (contains(key))
			{
				return m_PackedValArr.cbegin() + m_SparseArr[key];
			}
			return m_PackedValArr.cend();
		}		
		iterator find(KeyType key) noexcept
		{
			if (contains(key))
			{
				return m_PackedValArr.begin() + m_SparseArr[key];
			}
			return m_PackedValArr.end();
		}

	public:
		//Do not emplace the same element in the set twice, use try_emplace if this is a concern.
		template<typename... Args>
		requires std::is_constructible_v<Val, Args...>
		Val& emplace(KeyType element, Args&&... args) noexcept
		{
			ASSERT(!contains(element), "Element already in set!");

			if (element >= m_SparseArr.size())
			{
				m_SparseArr.resize(element + 1, INVALID_INDEX);
			}

			m_SparseArr[element] = static_cast<KeyType>(m_DenseArr.size());

			m_DenseArr.emplace_back(element);
			return m_PackedValArr.emplace_back(std::forward<Args>(args)...);
		}

		template<typename... Args>
		requires std::is_constructible_v<Val, Args...>
		std::pair<iterator, bool> try_emplace(KeyType element, Args&&... args) noexcept
		{
			if (contains(element))
			{
				return { m_PackedValArr.begin() + m_SparseArr[element], false};
			}
			
			emplace(element, std::forward<Args>(args)...);
			return { iterator{ m_PackedValArr.end() - 1 }, true };
		}

		template<typename... Args>
		requires std::is_constructible_v<Val, Args...>
		Val& get_or_emplace(KeyType element, Args&&... args) noexcept
		{
			if (!contains(element))
			{
				return emplace(element, std::forward<Args>(args)...);
			}

			return m_PackedValArr[m_SparseArr[element]];
		}

	public:
		//Do not erase an element that does not exist, use remove instead if this is a concern.
		void erase(KeyType element) noexcept
		{
			ASSERT(contains(element), "Element not in set!");

			if constexpr (std::is_trivially_copyable_v<Val>)
			{
				std::memcpy(&m_PackedValArr[m_SparseArr[element]], &m_PackedValArr.back(), sizeof(Val));
			}
			else if constexpr(Impl::MoveAssignmentVal<Val>)
			{
				m_PackedValArr[m_SparseArr[element]] = std::move(m_PackedValArr.back());
			}
			else if constexpr(Impl::MoveConstructVal<Val>)
			{
				m_PackedValArr[m_SparseArr[element]].~Val();
				new (&m_PackedValArr[m_SparseArr[element]]) Val(std::move(m_PackedValArr.back()));
			}
			
			m_DenseArr[m_SparseArr[element]] = m_DenseArr.back();
			
			m_SparseArr[m_DenseArr.back()] = m_SparseArr[element];
			m_SparseArr[element] = INVALID_INDEX;

			m_DenseArr.pop_back();
			m_PackedValArr.pop_back();
		}

		//Do not erase with iterator that's out of bounds
		iterator erase(const_iterator pos) noexcept
		{
			ASSERT(!(pos >= end() && pos < begin()), "Iterator out of bounds!");

			auto const distance{ std::distance(cbegin(), pos) };
			erase(m_DenseArr[val_index(pos)]);

			return begin() + distance;
		}

		//Do not erase with iterator range that's out of bounds
		iterator erase(const_iterator first, const_iterator last) noexcept
		{
			ASSERT(!(last > cend() || first < cbegin()) && first < last, "Iterator out of bounds!");
			ASSERT(first != last, "First == last erases nothing!");

			--last;
			while (last > first)
			{
				last = --erase(last);
			}

			return erase(last);
		}

		bool remove(KeyType element) noexcept
		{
			return contains(element) && (erase(element), true);
		}

	public:
		template <Impl::Compare<Val> Compare = std::less< >>
		void sort(Compare&& compare = { })
		{
			std::vector<size_t> copy(m_PackedValArr.size());
			std::iota(copy.begin(), copy.end(), size_t{ });

			std::sort(copy.begin(), copy.end(),
				[this, compare{ std::forward<Compare>(compare) }](const auto lhs, const auto rhs)
				{
					return std::invoke(compare, m_PackedValArr[lhs], m_PackedValArr[rhs]);
				});

			for (size_t pos{ 0 }; pos < copy.size(); ++pos) 
			{
				size_t curr = pos;
				size_t next = copy[curr];
				
				while (curr != next) 
				{
					swap_values(m_DenseArr[copy[curr]], m_DenseArr[copy[next]]);
					std::swap(m_SparseArr[m_DenseArr[copy[curr]]], m_SparseArr[m_DenseArr[copy[next]]]);

					std::swap(m_DenseArr[copy[curr]], m_DenseArr[copy[next]]);

					copy[curr] = curr;
					curr = next;
					next = copy[curr];
				}
			}
		}

		//Slower sort function without the copy, should be possibly to make faster  - TODO
		//template<typename Compare>
		//void sort(Compare compare)
		//{
		//	[The bottleneck is the actual sort here, likely due to the extra indirection.]
		//	std::sort(m_DenseArr.begin(), m_DenseArr.end(), std::move(compare));

		//	for (KeyType pos{ 0 }; pos < m_PackedValArr.size(); ++pos)
		//	{
		//		KeyType curr = pos;
		//		auto next = m_SparseArr[m_DenseArr[curr]];

		//		while (curr != next)
		//		{
		//			swap_values(m_DenseArr[curr], m_DenseArr[next]);
		//			m_SparseArr[m_DenseArr[curr]] = curr;

		//			curr = next;
		//			next = m_SparseArr[m_DenseArr[curr]];
		//		}
		//	}
		//}

		template <Impl::Compare<Val> Compare = std::less< >>
		[[nodiscard]] bool is_sorted(Compare&& compare = { }) const noexcept
		{
			return std::is_sorted(m_PackedValArr.begin(), m_PackedValArr.end(), std::forward<Compare>(compare));
		}

		//Emplace element in a sorted set, set should be sorted before using && element should not be in set yet.
		template<typename... Args, Impl::Compare<Val> Compare>
		requires std::is_constructible_v<Val, Args...> && Impl::MoveAssignmentVal<Val>
		iterator emplace_sorted(KeyType element, Compare&& compare, Args&&... args) noexcept
		{
			DEBUG_ASSERT(is_sorted(std::forward<Compare>(compare)), "Set must be sorted");
			ASSERT(!contains(element), "Element already in set!");

			if (element >= m_SparseArr.size())
			{
				m_SparseArr.resize(element + 1, INVALID_INDEX);
			}

			Val const value{ std::forward<Args>(args)... };

			auto const insertIt = lower_bound(value, std::forward<Compare>(compare));
			KeyType const denseIndex = static_cast<KeyType>(std::distance(m_PackedValArr.begin(), insertIt));

			m_DenseArr.insert(m_DenseArr.begin() + denseIndex, element);
			m_SparseArr[element] = denseIndex;

			m_PackedValArr.insert(insertIt, std::move(value));

			for (KeyType i = denseIndex + 1; i < m_DenseArr.size(); ++i)
			{
				m_SparseArr[m_DenseArr[i]] = i;
			}

			return m_PackedValArr.begin() + denseIndex;
		}
		//Emplace element in a sorted set, set should be sorted before using && element should not be in set yet.
		template<typename... Args>
		requires std::is_constructible_v<Val, Args...>&& Impl::MoveAssignmentVal<Val>
		iterator emplace_sorted(KeyType element, Args&&... args) noexcept
		{
			return emplace_sorted(element, std::less<>{ }, std::forward<Args>(args)...);
		}

		//Tries to emplace element in sorted set when it does not contain the element, set should be sorted.
		template<typename... Args, Impl::Compare<Val> Compare>
		requires std::is_constructible_v<Val, Args...>&& Impl::MoveAssignmentVal<Val>
		std::pair<iterator, bool> try_emplace_sorted(KeyType element, Compare&& compare, Args&&... args) noexcept
		{
			if (contains(element))
			{
				return { m_PackedValArr.begin() + m_SparseArr[element], false };
			}
			return { emplace_sorted(element, std::forward<Compare>(compare), std::forward<Args>(args)...), true };
		}
		//Tries to emplace element in sorted set when it does not contain the element, set should be sorted.
		template<typename... Args>
		requires std::is_constructible_v<Val, Args...>&& Impl::MoveAssignmentVal<Val>
		std::pair<iterator, bool> try_emplace_sorted(KeyType element, Args&&... args) noexcept
		{
			return try_emplace_sorted(element, std::less<>{ }, std::forward<Args>(args)...);
		}

	private:
		static constexpr KeyType INVALID_INDEX = std::numeric_limits<KeyType>::max();

		std::vector<KeyType> m_SparseArr{ };

		std::vector<KeyType> m_DenseArr{ };
		std::vector<Val> m_PackedValArr{ };

	private:
		template <typename IteratorType>
		[[nodiscard]] inline KeyType val_index(IteratorType pos) const noexcept
		{
			if constexpr (std::is_same_v<IteratorType, reverse_iterator>
				|| std::is_same_v<IteratorType, const_reserve_iterator>)
			{
				ASSERT(!(pos >= rend() && pos < rbegin()), "Reverse iterator out of bounds");
				return static_cast<KeyType>(rend() - 1 - pos);
			}
			else if constexpr (std::is_same_v<IteratorType, iterator>
					|| std::is_same_v<IteratorType, const_iterator>)
			{
				ASSERT(!(pos >= end() && pos < begin()), "Iterator out of bounds");
				return static_cast<KeyType>(pos - begin());
			}
		}

	private:
		//Should not swap elements that are not in the set, use try_swap if this is a concern
		void swap_values(KeyType el1, KeyType el2) noexcept
		{
			ASSERT(el1 != el2, "Should not try swap element with itself!");
			ASSERT(contains(el1) && contains(el2), "Set must contain elements!");

			if constexpr (std::is_trivially_copyable_v<Val>)
			{
				Val temp;
				std::memcpy(&temp, &m_PackedValArr[m_SparseArr[el1]], sizeof(Val));
				std::memcpy(&m_PackedValArr[m_SparseArr[el1]], &m_PackedValArr[m_SparseArr[el2]], sizeof(Val));
				std::memcpy(&m_PackedValArr[m_SparseArr[el2]], &temp, sizeof(Val));
			}
			else if constexpr (Impl::MoveAssignmentVal<Val>)
			{
				std::swap(m_PackedValArr[m_SparseArr[el1]], m_PackedValArr[m_SparseArr[el2]]);
			}
			else if constexpr (Impl::MoveConstructVal<Val>)
			{
				Val temp{ std::move(m_PackedValArr[m_SparseArr[el1]]) };
				m_PackedValArr[m_SparseArr[el1]].~Val();
				new (&m_PackedValArr[m_SparseArr[el1]]) Val(std::move(m_PackedValArr[m_SparseArr[el2]]));
				m_PackedValArr[m_SparseArr[el2]].~Val();
				new (&m_PackedValArr[m_SparseArr[el2]]) Val(std::move(temp));
			}
		}
		bool try_swap_values(KeyType el1, KeyType el2) noexcept
		{
			return (contains(el1) && contains(el2) && (swap_values(el1, el2), true));
		}
		//Should not swap elements that are not in the set, must use valid iterators
		void swap_values(const_iterator el1, const_iterator el2) noexcept
		{
			swap_values(sparse_index(el1), sparse_index(el2));
		}
	
	private:
		template <Impl::Compare<Val> Compare = std::less< >>
		iterator lower_bound(const Val& value, Compare&& compare = { }) noexcept
		{
			return std::lower_bound(m_PackedValArr.begin(), m_PackedValArr.end(), value, std::forward<Compare>(compare));
		}
		template <Impl::Compare<Val> Compare = std::less< >>
		const_iterator lower_bound(const Val& value, Compare&& compare = { }) const noexcept
		{
			return std::lower_bound(m_PackedValArr.begin(), m_PackedValArr.end(), value, std::forward<Compare>(compare));
		}
	};
}

#endif