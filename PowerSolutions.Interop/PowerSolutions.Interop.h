// PowerSolutions.Interop.h

#pragma once
#include <PowerSolutions/NetworkCase.h>

using namespace System;

namespace PowerSolutions{
	namespace Interop {
		/// <summary>
		/// ��ʾһ�����簸������<see cref="PowerSolutions.ObjectModel.NetworkCase" />��
		/// </summary>
		public ref class NetworkCase
		{
		private:
			ObjectModel::NetworkCase* nativeObject;
		public:
			void AddObject(ObjectModel::NetworkObject* obj);
			void AddBus(int index, double initialVoltage);
		};
	}
}
