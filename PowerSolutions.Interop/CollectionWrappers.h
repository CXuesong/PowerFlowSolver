#pragma once

namespace PowerSolutions
{
	namespace Interop
	{
		template <
			class TStlMap,
			class TWrappedKey = typename TStlMap::key_type,
			class TWrappedValue = typename TStlMap::mapped_type
		>
		public ref class ReadOnlyDictionaryWrapper
		: System::Collections::Generic::IDictionary < typename TWrappedKey, typename TWrappedValue >
			{
			internal:
				const TStlMap* nativeObject;
			public:
				typedef typename TStlMap::key_type key_type;
				typedef typename TStlMap::mapped_type value_type;
				typedef typename TStlMap::const_iterator TIterator;
				typedef System::Collections::Generic::KeyValuePair<TWrappedKey, TWrappedValue> myPair;
				/// <summary>
				/// 表示此字典的枚举数。
				/// </summary>
				ref class Enumerator
					: System::Collections::IDictionaryEnumerator,
					System::Collections::Generic::IEnumerator < myPair >
				{
					// 注意：此处使用 ref class 是由于 valuetype 不支持自定义 Dispose。
				internal:
					const TStlMap* nativeObject;
					TIterator* iterator;
				public:
					virtual property myPair Current
					{
						myPair get() sealed
						{
							return myPair(TWrappedKey((*iterator)->first), TWrappedValue((*iterator)->second));
						}
					}
					virtual bool MoveNext()
					{
						if (iterator == nullptr)
						{
							iterator = new TIterator;
							*iterator = nativeObject->begin();
						} else {
							iterator++;
						}
						bool test = *iterator != nativeObject->end();
						return *iterator != nativeObject->end();
					}
					virtual void Reset()
					{
						if (iterator != nullptr)
						{
							delete iterator;
							iterator = nullptr;
						}
					}
				private:
					virtual property Object^ Current1
					{
						Object^ get() sealed = System::Collections::IEnumerator::Current::get
						{
							return Current;
						}
					}
					virtual property System::Collections::DictionaryEntry Entry
					{
						System::Collections::DictionaryEntry get() sealed = IDictionaryEnumerator::Entry::get
						{
							return System::Collections::DictionaryEntry(TWrappedKey((*iterator)->first), TWrappedValue((*iterator)->second));
						}
					}
					virtual property Object^ Key
					{
						Object^ get() sealed = System::Collections::IDictionaryEnumerator::Key::get
						{
							return TWrappedKey((*iterator)->first);
						}
					}
					virtual property Object^ Value
					{
						Object^ get() sealed = System::Collections::IDictionaryEnumerator::Value::get
						{
							return TWrappedValue((*iterator)->second);
						}
					}
				internal:
					Enumerator(const TStlMap* native)
						: nativeObject(native), iterator(nullptr)
					{
						Reset();
					}
					!Enumerator()
					{
						Reset();
					}
					~Enumerator()
					{
						this->!Enumerator();
					}
				};

				virtual property int Count
				{
					int get() sealed { return nativeObject->size(); }
				}

				virtual property bool IsReadOnly
				{
					bool get() sealed { return true; }
				}

				virtual property System::Collections::Generic::ICollection<TWrappedKey>^ Keys
				{
					System::Collections::Generic::ICollection<TWrappedKey>^ get() sealed
					{
						throw gcnew NotImplementedException();
					}
				}

				virtual property System::Collections::Generic::ICollection<TWrappedValue>^ Values
				{
					System::Collections::Generic::ICollection<TWrappedValue>^ get() sealed
					{
						throw gcnew NotImplementedException();
					}
				}

				virtual property TWrappedValue default[TWrappedKey]
				{
					TWrappedValue get(TWrappedKey key) sealed
					{
						auto obj = nativeObject->find(key);
						if (obj != nativeObject->end())
							return TWrappedValue(obj->second);
						else
							throw gcnew System::Collections::Generic::KeyNotFoundException;
					}
					void set(TWrappedKey key, TWrappedValue value)
					{
						throw gcnew System::NotSupportedException;
					}
				}

					virtual bool ContainsKey(TWrappedKey key)
				{
					TIterator i = nativeObject->find((key_type)key);
					return i != nativeObject->end();
				}

				virtual bool TryGetValue(TWrappedKey key, TWrappedValue% value)
				{
					TIterator i = nativeObject->find(key);
					if (i != nativeObject->end())
					{
						value = TWrappedValue(i->second);
						return true;
					}
					return false;
				}

				virtual System::Collections::Generic::IEnumerator<myPair>^ GetEnumerator() = System::Collections::Generic::IEnumerable<myPair>::GetEnumerator
				{
					return gcnew Enumerator(nativeObject);
				}

			protected:
				ReadOnlyDictionaryWrapper(const TStlMap* native)
				{
					System::Diagnostics::Debug::Assert(native != nullptr);
					nativeObject = native;
				}
			private:
				virtual void Add(TWrappedKey key, TWrappedValue value) sealed
					= System::Collections::Generic::IDictionary < TWrappedKey, TWrappedValue >::Add
				{
					throw gcnew System::NotSupportedException();
				}

					virtual void Add(myPair item) sealed
					= System::Collections::Generic::ICollection < myPair >::Add
				{
					throw gcnew System::NotSupportedException();
				}

					virtual bool Remove(TWrappedKey key) sealed
					= System::Collections::Generic::IDictionary < TWrappedKey, TWrappedValue >::Remove
				{
					throw gcnew System::NotSupportedException();
				}

					virtual bool Remove(myPair item) sealed
					= System::Collections::Generic::ICollection < myPair >::Remove
				{
					throw gcnew System::NotSupportedException();
				}

					virtual void Clear() sealed
					= System::Collections::Generic::IDictionary < TWrappedKey, TWrappedValue >::Clear
				{
					throw gcnew System::NotSupportedException();
				}

					virtual bool Contains(myPair item) sealed
					= System::Collections::Generic::ICollection < myPair >::Contains
				{
					TWrappedValue obj;
					if (TryGetValue(item.Key, obj))
						return Object::Equals(item.Value, obj);
					else
						return false;
				}

					virtual void CopyTo(array<myPair>^ array, int arrayIndex) sealed
					= System::Collections::Generic::ICollection < myPair >::CopyTo
				{
					throw gcnew NotImplementedException();
				}

					virtual System::Collections::IEnumerator^ GetEnumerator1() sealed
					= System::Collections::IEnumerable::GetEnumerator
				{
					return gcnew Enumerator(nativeObject);
				}
			};

	}
}
