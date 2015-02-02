#pragma once

namespace PowerSolutions
{
	namespace Interop
	{
		template <class TKey, class TValue, class TIterator>
		public ref class ReadOnlyDictionaryWrapper
			: System::Collections::Generic::IDictionary < TKey, TValue >
		{
		internal:
			_NATIVE_PF Solution* nativeObject;
		public:
			typedef System::Collections::Generic::KeyValuePair<TKey, TValue> myPair;
			ref class Enumerator
				: System::Collections::IDictionaryEnumerator,
				System::Collections::Generic::IEnumerator < System::Collections::Generic::KeyValuePair<TKey, TValue> >
			{
			internal:
				_NATIVE_PF Solution* nativeObject;
				TIterator* iterator;
			public:
				virtual property System::Collections::Generic::KeyValuePair<TKey, TValue> Current
				{
					System::Collections::Generic::KeyValuePair<TKey, TValue> get() sealed
					{
						return System::Collections::Generic::KeyValuePair<TKey, TValue>(TKey((*iterator)->first), TValue((*iterator)->second));
					}
				}
				virtual bool MoveNext()
				{
					if (*iterator == nativeObject->NodeFlowEnd()) return false;
					iterator++;
					return *iterator != nativeObject->NodeFlowEnd();
				}
				virtual void Reset()
				{
					*iterator = nativeObject->NodeFlowBegin();
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
						return System::Collections::DictionaryEntry(TKey((*iterator)->first), TValue((*iterator)->second));
					}
				}
				virtual property Object^ Key
				{
					Object^ get() sealed = System::Collections::IDictionaryEnumerator::Key::get
					{
						return TKey((*iterator)->first);
					}
				}
				virtual property Object^ Value
				{
					Object^ get() sealed = System::Collections::IDictionaryEnumerator::Value::get
					{
						return TValue((*iterator)->second);
					}
				}
			internal:
				Enumerator(_NATIVE_PF Solution* native)
					: nativeObject(native),
					iterator(new TIterator)
				{
					Reset();
				}
				!Enumerator()
				{
					delete iterator;
				}
				~Enumerator()
				{
					this->!Enumerator();
				}
			};

			virtual property int Count
			{
				int get() sealed { return nativeObject->NodeFlowCount(); }
			}

			virtual property bool IsReadOnly
			{
				bool get() sealed { return true; }
			}

			virtual property System::Collections::Generic::ICollection<TKey>^ Keys
			{
				System::Collections::Generic::ICollection<TKey>^ get() sealed
				{
					throw gcnew NotImplementedException();
				}
			}

			virtual property System::Collections::Generic::ICollection<TValue>^ Values
			{
				System::Collections::Generic::ICollection<TValue>^ get() sealed
				{
					throw gcnew NotImplementedException();
				}
			}

			virtual property TValue default[TKey]
			{
				TValue get(TKey key) sealed
				{
					auto obj = nativeObject->NodeFlow(key);
					if (obj != nullptr)
						return TValue(*obj);
					else
						throw gcnew System::Collections::Generic::KeyNotFoundException;
				}
				void set(TKey key, TValue value)
				{
					throw gcnew System::NotSupportedException;
				}
			}

				virtual bool ContainsKey(TKey key)
			{
				return nativeObject->NodeFlow(key) != nullptr;
			}

			virtual void Add(TKey key, TValue value)
			{
				throw gcnew System::NotSupportedException();
			}

			virtual void Add(System::Collections::Generic::KeyValuePair<TKey, TValue> item)
			{
				throw gcnew System::NotSupportedException();
			}

			virtual bool Remove(TKey key)
			{
				throw gcnew System::NotSupportedException();
			}

			virtual bool Remove(System::Collections::Generic::KeyValuePair<TKey, TValue> item)
			{
				throw gcnew System::NotSupportedException();
			}

			virtual bool TryGetValue(TKey key, TValue% value)
			{
				auto obj = nativeObject->NodeFlow(key);
				if (obj != nullptr)
				{
					value = TValue(*obj);
					return true;
				}
				return false;
			}

			virtual void Clear()
			{
				throw gcnew System::NotSupportedException();
			}

			virtual bool Contains(System::Collections::Generic::KeyValuePair<TKey, TValue> item)
			{
				TValue obj;
				if (TryGetValue(item.Key, obj))
					return Object::Equals(item.Value, obj);
				else
					return false;
			}

			virtual void CopyTo(array<System::Collections::Generic::KeyValuePair<TKey, TValue>>^ array, int arrayIndex)
			{
				throw gcnew NotImplementedException();
			}

			virtual System::Collections::Generic::IEnumerator<System::Collections::Generic::KeyValuePair<TKey, TValue>>^ GetEnumerator() = System::Collections::Generic::IEnumerable<System::Collections::Generic::KeyValuePair<TKey, TValue>>::GetEnumerator
			{
				return gcnew Enumerator(nativeObject);
			}

			virtual System::Collections::IEnumerator^ GetEnumerator1() = System::Collections::IEnumerable::GetEnumerator
			{
				return gcnew Enumerator(nativeObject);
			}
		};
	}
}
