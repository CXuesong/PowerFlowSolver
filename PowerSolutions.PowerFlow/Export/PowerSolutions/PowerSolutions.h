#ifndef __POWERSOLUTIONS_H
#define __POWERSOLUTIONS_H

#include <complex>

namespace PowerSolutions {
	//类型重定义
	typedef std::complex<double> complexd;
	typedef unsigned char byte;

	//实用类型
	template < class TStlCollection, class TStlIterator = TStlCollection::iterator>
	class enumerable
	{
	protected:
		TStlCollection* m_Source;
	public:
		typedef typename TStlCollection::value_type value_type;
	public:
		TStlIterator begin() { return m_Collection->begin(); }
		TStlIterator end() { return m_Collection->end(); }
	public:
		enumerable(TStlCollection* source)
			: m_Source(source)
		{ }
	};

	template < class TStlCollection, class TStlIterator = TStlCollection::iterator>
	class readonly_collection : public enumerable<TStlCollection, TStlIterator>
	{
	public:
		typedef typename TStlCollection::size_type size_type;
	public:
		size_type size() const { return m_Source->size(); }
	public:
		readonly_collection(TStlCollection* source)
			: enumerable(source)
		{ }
	};

	template < class TStlMap, class TStlIterator = TStlMap::iterator>
	class readonly_map : public readonly_collection < TStlMap, TStlIterator>
	{
	public:
		typedef typename TStlMap::key_type key_type;
		typedef typename TStlMap::mapped_type mapped_type;
	public:
		mapped_type& at(const key_type& key)
		{
			return m_Source->at(key);
		}
		TStlIterator find(const key_type& key)
		{
			return m_Source->find(key);
		}
	public:
		readonly_map(TStlMap* source)
			: readonly_collection(source)
		{ }
	};
}

#if _DEBUG
//调试支持
#include <iostream>
namespace PowerSolutions {
	extern const char TraceFilePath[];
}
#endif

#endif //__POWERSOLUTIONS_H

