#pragma once



class ISimTable 
{

public:
	
	virtual ~ISimTable()	{};
	
	// F�gt ein neues Wertepaar in die tabel ein
	virtual void Add(double x , double y) = 0;
	// F�llt die Table mit einem Array
	virtual void Assign(unsigned int size_x,unsigned int size_y,double data[])=0;
	// Gibt eine Referenz auf die internen Daten zur�ck
	virtual  multi_array_ref<double,2> getValue() = 0;

};