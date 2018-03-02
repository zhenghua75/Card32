extern "C"
{
	__declspec (dllexport) __int16 __stdcall  SetDate(char *_Time);
    __declspec (dllexport) __int16 __stdcall  Put7CardEn(char CardNo[8],double Charge,int Ig);
	__declspec (dllexport) __int16 __stdcall  Put5CardEn(char CardNo[6],double Charge,int Ig);
	__declspec (dllexport) __int16 __stdcall  Put7CardOld(char CardNo[8],double Charge,int Ig);
	__declspec (dllexport) __int16 __stdcall  Put5CardOld(char CardNo[6],double Charge,int Ig);
	__declspec (dllexport) __int16 __stdcall  EnCard(char CardNo[6],double Charge,int Ig);
    __declspec (dllexport) __int16 __stdcall  WriteCharge(double Charge);
    __declspec (dllexport) __int16 __stdcall  WriteIg(int Ig);
    __declspec (dllexport) __int16 __stdcall  WriteCard(double Charge,int Ig);
	__declspec (dllexport) __int16 __stdcall  EmpPutCard(char CardNo[5]);
	__declspec (dllexport) __int16 __stdcall  EmpPutCardOld(char CardNo[5]);
    __declspec (dllexport) __int16 __stdcall  EmpReadCard(char CardSnr[33],char CardNo[6],bool &IsEn);
    __declspec (dllexport) __int16 __stdcall  RecycleCard();
    __declspec (dllexport) __int16 __stdcall  ReadCardSnr(char CardSnr[33]);
    __declspec (dllexport) __int16 __stdcall  ReadCardAll(char CardSnr[33],char CardNo[8],double &Charge,int &Ig,bool &IsEn);

}
