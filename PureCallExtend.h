#pragma once

struct PureCallBase
{
	PureCallBase() { mf(); }
	void mf()
	{

		//���� �����Լ� ������ ����Ű�� �ڵ�
		pvf();
	}
	virtual void pvf() = 0;
};

struct PureCallExtend : public PureCallBase
{
	PureCallExtend() {}
	virtual void pvf() {}
};