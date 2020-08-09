
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
			: public std::binary_function < std::pair<T, T>, std::pair<T, T>, bool >
		{
			bool operator()(std::pair<T, T> x, std::pair<T, T> y) const
			{
				return (x.first == y.first && x.second == y.second) ||
					(x.first == y.second && x.second == y.first);
			}
		};

		template<class TCheck, class T>
		bool IsKindOf(T* obj)
		{
			return dynamic_cast<TCheck*>(obj) != nullptr;
		}
	}
}

#endif

