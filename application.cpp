#include <iostream>
#include "ObjectModel.h"
#include "NetworkCase.h"

using namespace PowerSolutions::ObjectModel;
using namespace std;

int main()
{
	/*
	Bus  1, »·Íø-ÖÕ¶Ë, 1.00
	Bus  2, ÖÕ¶Ë, 1.00
	Bus  3, »·Íø-ÓÒ²à, 1.05
	Bus  4, »·Íø-×ó²à, 1.05
	#Bus  5, Test1, 1
	#Bus  6, Test2, 1
	#L 5,6,	0.260331, 0.495868, 0.051728
	T  1,2,	0, 0.1666666666666666666666, 0.886363636363636
	PVG  3,	0.2, 1.05
	SG  4,	1.05
	PQL  2,	0.5, 0.3
	PQL  4,	0.15, 0.1
	SA  2,	0, 0.05
	L  4,3,	0.260331, 0.495868, 0.051728
	L  1,4,	0.173554, 0.330579, 0.034486
	L  1,3,	0.130165, 0.247934, 0.025864
	*/
	NetworkCase network;
	network.Attach(make_shared<Bus>(1));
	network.Attach(make_shared<Bus>(2));
	network.Attach(make_shared<Bus>(3));
	network.Attach(make_shared<Bus>(4));
	network.Attach(make_shared<Bus>(5));
	setlocale(LC_ALL, "65001");
	return 0;
}