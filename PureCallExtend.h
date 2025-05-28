#pragma once

struct PureCallBase
{
	PureCallBase() { mf(); }
	void mf()
	{

		//순수 가상함수 에러를 일으키는 코드
		pvf();
	}
	virtual void pvf() = 0;
};

struct PureCallExtend : public PureCallBase
{
	PureCallExtend() {}
	virtual void pvf() {}
};