
#ifndef __POWERSOLUTIONS_UTILITY
#define __POWERSOLUTIONS_UTILITY

#include <functional>
#include <utility>

namespace PowerSolutions
{
	namespace Utility
	{
		//�ṩ��һ����˳���޹ص� pair �Ĺ�ϣ������
		template<class T>
		struct UnorderedPairHasher
			: public std::unary_function < std::pair<T, T>, size_t >
		{
			size_t operator()(std::pair<T, T> val) const
			{
				std::hash<T> hasher;
				return hasher(val.first) ^ hasher(val.second);
			}
		};

		//�ṩ��һ����˳���޹ص� pair ������ԱȽϺ�����
		template<class T>
		struct UnorderedPairEqualityComparer
			: public std::binary_function < const std::pair<T, T>, const std::pair<T, T>, bool >
		{
			bool operator()(const std::pair<T, T> x, const std::pair<T, T> y) const
			{
				return (x.first == y.first && x.second == y.second) ||
					(x.first == y.second && x.second == y.first);
			}
		};

		//�ṩ��һ����˳����ص� pair �Ĺ�ϣ������
		template<class T>
		struct OrderedPairHasher
			: public std::unary_function < const std::pair<T, T>, size_t >
		{
			size_t operator()(const std::pair<T, T> val) const
			{
				std::hash<T> hasher;
				return hasher(val.first) ^ hasher(val.second);
			}
		};

		//�ṩ��һ����˳����ص� pair ������ԱȽϺ�����
		template<class T>
		struct OrderedPairEqualityComparer
			: public std::binary_function < const std::pair<T, T>, const std::pair<T, T>, bool >
		{
			bool operator()(const std::pair<T, T> x, const std::pair<T, T> y) const
			{
				return (x.first == y.first && x.second == y.second);
			}
		};

		template<class TCheck, class T>
		bool IsKindOf(const T* obj)
		{
			return dynamic_cast<const TCheck*>(obj) != nullptr;
		}

		template<class TEnum>
		bool HasFlag(TEnum v, TEnum testV)
		{

		}
	}
}

#endif

