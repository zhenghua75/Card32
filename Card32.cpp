// Card32.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Card32.h"
#include "mwrf32.h"
#include <string>
using namespace std; 
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

struct M1CARD
{
   	int port					;
	long baud					;
    HANDLE icdev                ;
	unsigned char Mode			;
	unsigned long Snr			;
	char *EnKey					;//加密键
	char *SecKey				;//当前扇区密码
	unsigned char SecNr         ;    
	char *ShopNo				;
	unsigned char Adr          ;//= 0;
	//int st                  ;//= 0;
	char *akey1 ;
	char *bkey1 ;
	char *aKey1Old ;
	char *bkey1Old ;
    char *akey0 ;
    char *bkey0 ;
};
__int16 quit(HANDLE icdev)
{	
	if(icdev != NULL)
	{
		if( icdev>0)
		{
			rf_halt(icdev);
			rf_reset(icdev, 10);
			rf_exit(icdev);
			//icdev = NULL;
		}
	}
	return 0;
}

extern "C" __declspec (dllexport) __int16 __stdcall SetDate(char *_Time)
{
    __int16 st = 0;
	HANDLE icdev = rf_init(0, 9600);
	if(icdev<0)
	{
		return 1;
	}
	st = rf_settimehex(icdev,_Time);
	if(st)
	{
		return 33;
	}
	st=rf_disp_mode(icdev,0x01);//设为显示时间
	if(st)
	{
		return 14;
	}
	//退出
	quit(icdev);
	return 0;
}
__int16 InitCard (M1CARD &m1)
{
   	m1.port					= 0;
	m1.baud					= 9600;	
    m1.icdev                = NULL;	
	m1.EnKey        = "12345678";    
	
	m1.ShopNo    = "00000000000000000000000000000101";//面包工坊
	m1.akey1              = "B648A7F3021C";
	m1.bkey1              = "C03F5591EB08";
	m1.aKey1Old           = "A3D4C68CD9E5";
	m1.bkey1Old           = "C03F5591EB08";
    m1.akey0              = "FFFFFFFFFFFF";
    m1.bkey0              = "FFFFFFFFFFFF";
    __int16 st;
	unsigned char _Status[30];
	memset(_Status,0,30);
	m1.icdev = rf_init(m1.port, m1.baud);
	if(m1.icdev<0)
	{
		return 1;
	}
	st = rf_get_status(m1.icdev,_Status);
	if(st)
	{
		return 1;
	}
    //寻卡	
    st = rf_reset(m1.icdev, 10);//'射频读写模块复位
    if(st)
        return 34;
	m1.Mode = 1;
	unsigned __int16 TagType;
	st = rf_request(m1.icdev,m1.Mode,&TagType);
	if(st)
		return 2;	
	st = rf_anticoll(m1.icdev,0,&m1.Snr);
	if(st)
		return 3;
	unsigned char _Size;
    st = rf_select(m1.icdev,m1.Snr,&_Size);
	if(st)	
		return 4;
    return 0;
}

__int16 Check(M1CARD m1)
{
	__int16 st;
	unsigned char key[7];
	memset(key,0,7);
	a_hex(m1.SecKey,key,12);	
    st = rf_load_key(m1.icdev, m1.Mode, m1.SecNr, key);
	if(st)
		return 5;
    st = rf_authentication(m1.icdev, m1.Mode, m1.SecNr);
	if(st)
		return 7;
    return 0;
}

__int16 WriteData(M1CARD m1,char _Data[33])//写数据
{
    __int16 st;
	unsigned char ptrSource[33];
	memset(ptrSource,0,33);
	a_hex(_Data,ptrSource,32);

	unsigned char ptrDest[33];
	memset(ptrDest,0,33);
	
	unsigned int msgLen = 32;
	rf_encrypt((unsigned char*)m1.EnKey,ptrSource,msgLen,ptrDest);

    st = rf_write(m1.icdev, m1.Adr,ptrDest);
	if(st)
		return 39;
	st = rf_check_write(m1.icdev,m1.Snr,m1.Mode,m1.Adr,ptrDest);
	if(st)
		return 40;
    return 0;
}
__int16 UnEnWriteData(M1CARD m1,char _Data[33])//写数据
{
    __int16 st;
	unsigned char _Temp[33];
	memset(_Temp,0,33);
	a_hex(_Data,_Temp,32);
    st = rf_write(m1.icdev, m1.Adr,_Temp);
	if(st)
		return 39;
	st = rf_check_write(m1.icdev,m1.Snr,m1.Mode,m1.Adr,_Temp);
	if(st)
		return 40;
    return 0;
}
__int16 ReadData(M1CARD m1,char _Data[33])//读数据
{
    __int16 st;
	unsigned char ptrSource[33];
	memset(ptrSource,0,33);
	unsigned char ptrDest[33];
	memset(ptrDest,0,33);
    st = rf_read(m1.icdev, m1.Adr, ptrSource);
	if(st)
		return 41;
	//
	unsigned int msglen = 32;
	rf_decrypt((unsigned char*)m1.EnKey,ptrSource,msglen,ptrDest);
	hex_a(ptrDest,_Data,16);
    return 0;
}
__int16 UnEnReadData(M1CARD m1,char _Data[33])//读未加密数据
{
    __int16 st;
    st = rf_read_hex(m1.icdev, m1.Adr, _Data);
	if(st)
		return 41;
    return 0;
}
__int16 PutCard(char CardNo[33],char Charge[33],char Ig[33])//发卡
{		       
    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	//0扇区操作
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
	if(st)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}
	//m1.Mode = 4;
	//st = Check0B(m1);
	//if(st)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}	
	char CardSnr[33];
	memset(CardSnr,0,33);
	m1.Adr = 0;
	st = UnEnReadData(m1,CardSnr);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 1;
	st = WriteData(m1,CardSnr);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 2;
	st = WriteData(m1,m1.ShopNo);//面包工坊已加密
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
    //装载密码A  A3D4C68CD9E5
    m1.Mode = 0;
	m1.SecKey = m1.aKey1Old;
	m1.SecNr = 1;
	st = Check(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}	
	m1.Adr = 4;
    st = WriteData(m1,CardNo);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 5;
    st = WriteData(m1,Charge);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 6;
    st = WriteData(m1,Ig);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	unsigned char akeynew[7];
	memset(akeynew,0,7);
	a_hex(m1.akey1,akeynew,12);
	unsigned char bkeynew[7];   
	memset(bkeynew,0,7);
	a_hex(m1.bkey1,bkeynew,12);
    st = rf_changeb3(m1.icdev, 1, akeynew, 3, 3, 3, 3, 0, bkeynew);
	if(st)
	{
		quit(m1.icdev);
		return 11;
	}    
    st = rf_beep(m1.icdev, 5);
    //'取消设备
    quit(m1.icdev);
	return 0;               
}
extern "C" __declspec (dllexport) __int16 __stdcall  Put7CardEn(char *_CardNo,double _Charge,int _Ig)
{
	//卡号
	char CardNo[33];
	memset(CardNo,0,33);
	string _SCardNoAdd(25,'0');
	string _SCardNo(_CardNo);
	//_SDataGroup += _SCardNo;	
	_SCardNoAdd+=_SCardNo;
	//_SCardNo += _SCardNoAdd;
	//_SCardNo.copy(CardNo,32,0);
	_SCardNoAdd.copy(CardNo,32,0);
	//余额
	char Charge[33];
	memset(Charge,0,33);
	char   Buffer[33];
	memset(Buffer,0,33);
    sprintf(Buffer,"%.2f",_Charge);  

	string _SCharge(Buffer);	
	string::size_type _Pos;
	_Pos = _SCharge.find(".",0);
	string _iCharge = _SCharge.substr(0,_Pos);
	string _iDotCharge = _SCharge.substr(_Pos+1,2);

	string _SChargeAdd(30-_iCharge.length(),'F');
	_iCharge += _SChargeAdd;
	_iCharge += _iDotCharge;	
	_iCharge.copy(Charge,32,0);


	//积分
	char Ig[33];
	memset(Ig,0,33);
	if(_Ig<0)
		_Ig = 0;
	char _CIg[32];
	itoa(_Ig,_CIg,10);
	string _SIg(_CIg);
	string _SIgAdd(32-_SIg.length(),'F');
	_SIg += _SIgAdd;	
	_SIg.copy(Ig,32,0);

	return PutCard(CardNo,Charge,Ig);
}
extern "C" __declspec (dllexport) __int16 __stdcall  Put5CardEn(char *_CardNo,double _Charge,int _Ig)
{
	//卡号
	char CardNo[33];
	memset(CardNo,0,33);
	string _SCardNOAdd(27,'0');
	string _SCardNo(_CardNo);	
	_SCardNo += _SCardNOAdd;
	_SCardNo.copy(CardNo,32,0);

	//余额
	char Charge[33];
	memset(Charge,0,33);
	char   Buffer[33];
	memset(Buffer,0,33);
    sprintf(Buffer,"%.2f",_Charge);  

	string _SCharge(Buffer);	
	string::size_type _Pos;
	_Pos = _SCharge.find(".",0);
	string _iCharge = _SCharge.substr(0,_Pos);
	string _iDotCharge = _SCharge.substr(_Pos+1,2);

	string _SChargeAdd(30-_iCharge.length(),'F');
	_iCharge += _SChargeAdd;
	_iCharge += _iDotCharge;	
	_iCharge.copy(Charge,32,0);


	//积分
	char Ig[33];
	memset(Ig,0,33);
	if(_Ig<0)
		_Ig = 0;
	char _CIg[32];
	itoa(_Ig,_CIg,10);
	string _SIg(_CIg);
	string _SIgAdd(32-_SIg.length(),'F');
	_SIg += _SIgAdd;	
	_SIg.copy(Ig,32,0);

	return PutCard(CardNo,Charge,Ig);

}
extern "C" __declspec (dllexport) __int16 __stdcall  Put7CardOld(char *_CardNo,double _Charge,int _Ig)
{	
	__int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}	
    //装载密码A  A3D4C68CD9E5
    m1.Mode = 0;
	m1.SecKey = m1.aKey1Old;
	m1.SecNr = 1;
	st = Check(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}	
	char _Data[33];
	memset(_Data,0,33);
	//卡号
	string _SDataGroup(25,'0');
	string _SCardNo(_CardNo);
	_SDataGroup += _SCardNo;	
	_SDataGroup.copy(_Data,32,0);

	m1.Adr = 4;
    st = UnEnWriteData(m1,_Data);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}

	//余额
	unsigned long _LCharge = (_Charge+0.005) * 100;    
    st = rf_initval(m1.icdev, 5, _LCharge);
	if(st)
	{
		quit(m1.icdev);
		return 20;
	}
	//积分
	if(_Ig < 0)
		_Ig = 0;
	unsigned long _LIg = _Ig;
    st = rf_initval(m1.icdev, 6, _LIg);
	if(st)
	{
		quit(m1.icdev);
		return 21;		
	}

	//改密码
	unsigned char akeynew[7];
	memset(akeynew,0,7);
	a_hex(m1.akey1,akeynew,12);
	unsigned char bkeynew[7];   
	memset(bkeynew,0,7);
	a_hex(m1.bkey1,bkeynew,12);
    st = rf_changeb3(m1.icdev, 1, akeynew, 3, 3, 3, 3, 0, bkeynew);
	if(st)
	{
		quit(m1.icdev);
		return 11;
	}    
    st = rf_beep(m1.icdev, 5);
    //'取消设备
    quit(m1.icdev);
	return 0;   
}
extern "C" __declspec (dllexport) __int16 __stdcall  Put5CardOld(char *_CardNo,double _Charge,int _Ig)
{	
	__int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}	
    //装载密码A  A3D4C68CD9E5
    m1.Mode = 0;
	m1.SecKey = m1.aKey1Old;
	m1.SecNr = 1;
	st = Check(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}	
	char _Data[33];
	memset(_Data,0,33);
	//卡号
	string _SCardNoAdd(27,'0');
	string _SCardNo(_CardNo);
	//_SDataGroup += _SCardNo;	
	_SCardNo += _SCardNoAdd;
	_SCardNo.copy(_Data,32,0);
	m1.Adr = 4;
    st = UnEnWriteData(m1,_Data);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	//余额
	char   Buffer[32];  
    sprintf(Buffer,"%.2f",_Charge);  

	string _SCharge(Buffer);	
	string::size_type _Pos;
	_Pos = _SCharge.find(".",0);
	string _iCharge = _SCharge.substr(0,_Pos);
	string _iDotCharge = _SCharge.substr(_Pos+1,2);

	string _SChargeAdd(30-_iCharge.length(),'F');
	_iCharge += _SChargeAdd;
	_iCharge += _iDotCharge;	
	_iCharge.copy(_Data,32,0);

	m1.Adr = 5;
	st = UnEnWriteData(m1,_Data);
	if(st)
	{
		quit(m1.icdev);
		return 21;		
	}
	//积分
	if(_Ig<0)
		_Ig = 0;
	char _CIg[32];
	itoa(_Ig,_CIg,10);
	string _SIg(_CIg);
	string _SIgAdd(32-_SIg.length(),'F');
	_SIg += _SIgAdd;	
	_SIg.copy(_Data,32,0);
	m1.Adr = 6;
	st = UnEnWriteData(m1,_Data);

	//改密码
	unsigned char akeynew[7];
	memset(akeynew,0,7);
	a_hex(m1.akey1,akeynew,12);
	unsigned char bkeynew[7];   
	memset(bkeynew,0,7);
	a_hex(m1.bkey1,bkeynew,12);
    st = rf_changeb3(m1.icdev, 1, akeynew, 3, 3, 3, 3, 0, bkeynew);
	if(st)
	{
		quit(m1.icdev);
		return 11;
	}    
    st = rf_beep(m1.icdev, 5);
    //'取消设备
    quit(m1.icdev);
	return 0;   
}

__int16 EncryptCard(char CardNo[33],char Charge[33],char Ig[33])//加密卡
{		       
    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	//0扇区操作
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
	if(st)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}
	//m1.Mode = 4;
	//st = Check0B(m1);
	//if(st)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}	

	char CardSnr[33];
    memset(CardSnr,0,33);
	m1.Adr = 0;
	st = UnEnReadData(m1,CardSnr);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 1;
	st = WriteData(m1,CardSnr);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 2;
	st = WriteData(m1,m1.ShopNo);//面包工坊已加密
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
    //装载密码A  A3D4C68CD9E5
    m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
	st = Check(m1);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}	
	m1.Adr = 4;
    st = WriteData(m1,CardNo);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 5;
    st = WriteData(m1,Charge);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 6;
    st = WriteData(m1,Ig);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
    st = rf_beep(m1.icdev, 5);
    //'取消设备
    quit(m1.icdev);
	return 0;               
}
extern "C" __declspec (dllexport) __int16 __stdcall  EnCard(char *_CardNo,double _Charge,int _Ig)//加密卡
{
	//卡号
	char CardNo[33];
	memset(CardNo,0,33);
	string _SCardNo(_CardNo);	
	//int len = _SCardNo.length;
	string _SCardNOAdd(32-_SCardNo.length(),'0');
	if(_SCardNo.length()>5)
	{
		_SCardNOAdd += _SCardNo;	
		_SCardNOAdd.copy(CardNo,32,0);
		
	}
	else
	{
		_SCardNo += _SCardNOAdd;
		_SCardNo.copy(CardNo,32,0);
	}
	//余额
	char Charge[33];
	memset(Charge,0,33);
	char   Buffer[33];
	memset(Buffer,0,33);
    sprintf(Buffer,"%.2f",_Charge);  

	string _SCharge(Buffer);	
	string::size_type _Pos;
	_Pos = _SCharge.find(".",0);
	string _iCharge = _SCharge.substr(0,_Pos);
	string _iDotCharge = _SCharge.substr(_Pos+1,2);

	string _SChargeAdd(30-_iCharge.length(),'F');
	_iCharge += _SChargeAdd;
	_iCharge += _iDotCharge;	
	_iCharge.copy(Charge,32,0);


	//积分
	char Ig[33];
	memset(Ig,0,33);
	if(_Ig<0)
		_Ig = 0;
	char _CIg[32];
	itoa(_Ig,_CIg,10);
	string _SIg(_CIg);
	string _SIgAdd(32-_SIg.length(),'F');
	_SIg += _SIgAdd;	
	_SIg.copy(Ig,32,0);

	return EncryptCard(CardNo,Charge,Ig);
}
extern "C" __declspec (dllexport) __int16 __stdcall  WriteCharge(double _Charge)//写余额
{
	//余额
	char Charge[33];
	memset(Charge,0,33);
	char   Buffer[33];
	memset(Buffer,0,33);
    sprintf(Buffer,"%.2f",_Charge);  

	string _SCharge(Buffer);	
	string::size_type _Pos;
	_Pos = _SCharge.find(".",0);
	string _iCharge = _SCharge.substr(0,_Pos);
	string _iDotCharge = _SCharge.substr(_Pos+1,2);

	string _SChargeAdd(30-_iCharge.length(),'F');
	_iCharge += _SChargeAdd;
	_iCharge += _iDotCharge;	
	_iCharge.copy(Charge,32,0);

    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	
	m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
    st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	m1.Adr = 5;
	st	= WriteData(m1,Charge);
    if(st)
	{
		quit(m1.icdev);
		return st;
	}      
    st = rf_beep(m1.icdev, 5);
	quit(m1.icdev);
    return 0;
}

extern "C" __declspec (dllexport) __int16 __stdcall  WriteIg(int _Ig)//写积分
{
	char Ig[33];
	memset(Ig,0,33);
	if(_Ig<0)
		_Ig = 0;
	char _CIg[32];
	itoa(_Ig,_CIg,10);
	string _SIg(_CIg);
	string _SIgAdd(32-_SIg.length(),'F');
	_SIg += _SIgAdd;	
	_SIg.copy(Ig,32,0);

    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return 0;
	}
	
	m1.Mode = 4;
    m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
    st = Check(m1);
    if(st!=0)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 6;
	st = WriteData(m1,Ig);
    if(st!=0)
	{
		quit(m1.icdev);
		return st;
	}
    st = rf_beep(m1.icdev, 5);
	quit(m1.icdev);
	return 0;
}

extern "C" __declspec (dllexport) __int16 __stdcall  WriteCard(double _Charge,int _Ig)//写卡
{
		//余额
	char Charge[33];
	memset(Charge,0,33);
	char   Buffer[33];
	memset(Buffer,0,33);
    sprintf(Buffer,"%.2f",_Charge);  

	string _SCharge(Buffer);	
	string::size_type _Pos;
	_Pos = _SCharge.find(".",0);
	string _iCharge = _SCharge.substr(0,_Pos);
	string _iDotCharge = _SCharge.substr(_Pos+1,2);

	string _SChargeAdd(30-_iCharge.length(),'F');
	_iCharge += _SChargeAdd;
	_iCharge += _iDotCharge;	
	_iCharge.copy(Charge,32,0);

	char Ig[33];
	memset(Ig,0,33);
	if(_Ig<0)
		_Ig = 0;
	char _CIg[32];
	itoa(_Ig,_CIg,10);
	string _SIg(_CIg);
	string _SIgAdd(32-_SIg.length(),'F');
	_SIg += _SIgAdd;	
	_SIg.copy(Ig,32,0);

    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	
	m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
    st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	m1.Adr = 5;
	st = WriteData(m1,Charge);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	m1.Adr = 6;
	st = WriteData(m1,Ig);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}              
    st = rf_beep(m1.icdev, 5);
	quit(m1.icdev);
	return 0;
}
__int16 ReadCard(char CardSnr[33],char CardNo[33],char Charge[33],char Ig[33],bool &IsEn)//读卡
{
    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}  
	//m1.Mode = 4;
	//st = Check0B(m1);
    //if(st>0)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}  	
	char EnCardSnr[33];
	memset(EnCardSnr,0,33);	
	m1.Adr = 1;
	st = ReadData(m1,EnCardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	} 
	m1.Adr = 0;
	st = UnEnReadData(m1,CardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	st = strcmp((const char*)EnCardSnr,(const char*)CardSnr);
	IsEn = true;
	if(st!=0)
	{
		//未加密的卡
		IsEn = false;
	}  
	m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	if(IsEn)
	{
		m1.Adr = 4;
		st = ReadData(m1,CardNo);
		if(st>0)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Adr = 5;
		st = ReadData(m1,Charge);
		if(st>0)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Adr = 6;
		st = ReadData(m1,Ig);
		if(st>0)
		{
			quit(m1.icdev);
			return st;
		}
	}
	else
	{
		m1.Adr = 4;
		st = UnEnReadData(m1,CardNo);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}

		m1.Adr = 5;
		st = UnEnReadData(m1,Charge);			
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Adr = 6;
		st = UnEnReadData(m1,Ig);			
		if(st)
		{
			quit(m1.icdev);
			return 18;
		}
	}
    st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
    return 0;
}


extern "C" __declspec (dllexport) __int16 __stdcall  ReadCardAll(char CardSnr[33],char CardNo[8],double &Charge,int &Ig,bool &IsEn)//读卡
{
    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}  
	//m1.Mode = 4;
	//st = Check0B(m1);
    //if(st>0)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}  	
	char EnCardSnr[33];
	memset(EnCardSnr,0,33);	
	m1.Adr = 1;
	st = ReadData(m1,EnCardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	} 
	//char CardSnr[33];
	memset(CardSnr,0,33);
	m1.Adr = 0;
	st = UnEnReadData(m1,CardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	st = strcmp((const char*)EnCardSnr,(const char*)CardSnr);
	//*_CardSnr = CardSnr;
	IsEn = true;
	if(st!=0)
	{
		//未加密的卡
		IsEn = false;
	}  
	//*_IsEn = IsEn;
	m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	char _CardNo[33];
	memset(_CardNo,0,33);
	char _Charge[33];
	memset(_Charge,0,33);
	char _Ig[33];
	memset(_Ig,0,33);
	m1.Adr = 4;
	if(IsEn)
	{		
		st = ReadData(m1,_CardNo);
	}
	else
	{
		st = UnEnReadData(m1,_CardNo);
	}
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
	string SCardNo((char*)_CardNo);
	string _Temp = SCardNo.substr(25,7);
	bool Is5 = false;
	//char k7[8];
	memset(CardNo,0,8);
	//SCardNo.copy(*_CardNo,
	_Temp.copy(CardNo,7,0);
	//*_CardNo = k7;
	if(_Temp == "0000000")
	{
		//char k5[6];
		memset(CardNo,0,8);
		//memset(k5,0,6);
		SCardNo.copy(CardNo,5,0);
		//*_CardNo = k5;
		Is5 = true;
	}	
	if(IsEn)
	{
		m1.Adr = 5;
		st = ReadData(m1,_Charge);
		if(st>0)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Adr = 6;
		st = ReadData(m1,_Ig);
		if(st>0)
		{
			quit(m1.icdev);
			return st;
		}
	}
	else
	{
		if(Is5)
		{
			//5位卡
			m1.Adr = 5;
			st = UnEnReadData(m1,_Charge);			
			if(st)
			{
				quit(m1.icdev);
				return st;
			}

			
			m1.Adr = 6;
			st = UnEnReadData(m1,_Ig);			
			if(st)
			{
				quit(m1.icdev);
				return 18;
			}		
		}
		else
		{
			//7位卡
			unsigned long _Value=0;
			st = rf_readval(m1.icdev, 5, &_Value);
			if(st)
			{
				quit(m1.icdev);
				return 17;
			}
			double _DValue = (double)_Value/100;
			Charge = _DValue;
			_Value=0;
			st = rf_readval(m1.icdev, 6, &_Value);
			if(st)
			{
				quit(m1.icdev);
				return 18;
			}
			Ig = (int)_Value;
		}    
	}
	if(IsEn || Is5)
	{
		string _SCharge(_Charge);
		string _Pattern = "F";
		string::size_type _Pos;	
		_Pos = _SCharge.find(_Pattern,0);	
		string _SChargeAdd = _SCharge.substr(0,_Pos);
		string _SDotCharge = _SCharge.substr(30,2);	
		_SChargeAdd += ".";
		_SChargeAdd += _SDotCharge;
		double _DCharge = atof(_SChargeAdd.c_str());
		Charge = _DCharge;

		string _SIg(_Ig);
		_Pos = _SIg.find(_Pattern,0);
		string _SIg1 = _SIg.substr(0,_Pos);
		int _IIg = atoi(_SIg1.c_str());
		Ig = _IIg;
	}
    st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
    return 0;
}

extern "C" __declspec (dllexport) __int16 __stdcall  EmpPutCard(char *_CardNo)//员工卡发卡
{
	char CardNo[33];
	memset(CardNo,0,33);
	
	string _SCardNo(_CardNo);
	string _SDataGroup(27,'0');
	_SDataGroup += "F";
	_SDataGroup += _SCardNo;
	_SDataGroup.copy(CardNo,32,0);

    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
	if(st>0)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}
	//m1.Mode = 4;
	//st = Check0B(m1);
	//if(st>0)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}	

	char CardSnr[33];
    memset(CardSnr,0,33);
	m1.Adr = 0;
	st = UnEnReadData(m1,CardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 1;
	st = WriteData(m1,CardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Adr = 2;
	st = WriteData(m1,m1.ShopNo);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
	m1.Mode = 0;
	m1.SecKey = m1.aKey1Old;
	m1.SecNr = 1;
    st = Check(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}	
	m1.Adr = 4;
    st = WriteData(m1,CardNo);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
    //'改密码
	unsigned char akeynew[7];//={0xB6,0x48,0xA7,0xF3,0x02,0x1C};
	unsigned char bkeynew[7];//={0xC0,0x3F,0x55,0x91,0xEB,0x08};    
	a_hex(m1.akey1,akeynew,12);
	a_hex(m1.bkey1,bkeynew,12);
    st = rf_changeb3(m1.icdev, 1, akeynew, 3, 3, 3, 3, 0, bkeynew);
	if(st)
	{
		quit(m1.icdev);
		return 11;
	}    
    st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
	return 0;               
}
extern "C" __declspec (dllexport) __int16 __stdcall  EmpPutCardOld(char *_CardNo)//员工卡发卡
{
	char CardNo[33];
	memset(CardNo,0,33);
	
	string _SCardNo(_CardNo);
	string _SDataGroup(27,'0');
	_SDataGroup += "F";
	_SDataGroup += _SCardNo;
	_SDataGroup.copy(CardNo,32,0);

    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
	
	m1.Mode = 0;
	m1.SecKey = m1.aKey1Old;
	m1.SecNr = 1;
    st = Check(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}	
	m1.Adr = 4;
    st = UnEnWriteData(m1,CardNo);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}
    //'改密码
	unsigned char akeynew[7];//={0xB6,0x48,0xA7,0xF3,0x02,0x1C};
	unsigned char bkeynew[7];//={0xC0,0x3F,0x55,0x91,0xEB,0x08};    
	a_hex(m1.akey1,akeynew,12);
	a_hex(m1.bkey1,bkeynew,12);
    st = rf_changeb3(m1.icdev, 1, akeynew, 3, 3, 3, 3, 0, bkeynew);
	if(st)
	{
		quit(m1.icdev);
		return 11;
	}    
    st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
	return 0;               
}

extern "C" __declspec (dllexport) __int16 __stdcall  EmpReadCard(char CardSnr[33],char CardNo[6],bool &IsEn)//员工卡读卡OK
{
    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}  
	//m1.Mode = 4;
	//st = Check0B(m1);
    //if(st>0)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}  	
	char EnCardSnr[33];
	memset(EnCardSnr,0,33);
	m1.Adr = 1;
	st = ReadData(m1,EnCardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	} 
	memset(CardSnr,0,33);
	m1.Adr = 0;
	st = UnEnReadData(m1,CardSnr);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	} 
	st = strcmp((const   char*)EnCardSnr,(const   char*)CardSnr);
	IsEn = true;
	if(st!=0)
	{
		//未加密的卡
		IsEn = false;
	}  
	//*_IsEn = IsEn;
	//*_CardSnr = CardSnr;

	m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	char _CardNo[33];
	memset(_CardNo,0,33);
	if(IsEn)
	{
		m1.Adr = 4;
		st = ReadData(m1,_CardNo);
		if(st>0)
		{
			quit(m1.icdev);
			return st;
		}  
	}
	else
	{    
        m1.Adr = 4;
		st = UnEnReadData(m1,_CardNo);
		if(st)
		{
			quit(m1.icdev);
			return 16;
		}
	}
	string _SData(_CardNo);
	string _Temp = _SData.substr(27,5);
    if( _Temp == "00000")
	{
		_Temp = _SData.substr(0,5);
	}
	
	//*_CardNo = CardNo;
	//char k5[6];
	memset(CardNo,0,6);
	_Temp.copy(CardNo,5,0);
	//*_CardNo = k5;
    st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
    return 0;
}
extern "C" __declspec (dllexport) __int16 __stdcall  RecycleCard()//卡回收
{
    __int16 st;
    M1CARD m1;
    st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
	if(st>0)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}   
	char _Data[33] = "00000000000000000000000000000000";
	m1.Adr = 1;
	st = UnEnWriteData(m1,_Data);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}   
	m1.Adr = 2;
	st = UnEnWriteData(m1,_Data);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	m1.Mode = 4;
	m1.SecKey = m1.bkey1;
	m1.SecNr = 1;
    st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		return st;
	}  
	m1.Adr = 4;
    st = UnEnWriteData(m1,_Data);
	if(st)
	{
		quit(m1.icdev);
		return 19;
	}
	m1.Adr = 5;
    st = UnEnWriteData(m1,_Data);
	if(st)
	{
		quit(m1.icdev);
		return 20;
	}
	m1.Adr = 6;
    st = UnEnWriteData(m1,_Data);
	if(st)
	{
		quit(m1.icdev);
		return 21;
	}
	unsigned char akeynew[7];// = {0xA3,0xD4,0xC6,0x8C,0xD9,0xE5};
	unsigned char bkeynew[7];// = {0xB0,0x1B,0x4C,0x49,0xA3,0xD3};
	a_hex(m1.aKey1Old,akeynew,12);
	a_hex(m1.bkey1Old,bkeynew,12);
	//unsigned char _SecNr = 1;
    st = rf_changeb3(m1.icdev, 1, akeynew, 0, 0, 0, 1, 0, bkeynew);
	if(st)
	{
		quit(m1.icdev);
		return 11;
	}    
    st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
	return 0;    
}
extern "C" __declspec (dllexport) __int16 __stdcall  ReadCardSnr(char CardSnr[33])
{
    __int16 st;
    M1CARD m1;
	st = InitCard(m1);
	if(st>0)
	{
		quit(m1.icdev);
		return st;
	}    
	m1.Mode = 0;
	m1.SecKey = m1.akey0;
	m1.SecNr = 0;
	st = Check(m1);
    if(st>0)
	{
		quit(m1.icdev);
		st = InitCard(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
		m1.Mode = 0;
		m1.SecKey = m1.aKey1Old;
		m1.SecNr = 0;
		st = Check(m1);
		if(st)
		{
			quit(m1.icdev);
			return st;
		}
	}  
	//m1.Mode = 4;
	//st = Check0B(m1);
    //if(st>0)
	//{
	//	quit(m1.icdev);
	//	return st;
	//}  	
	//char CardSnr[33];
	memset(CardSnr,0,33);
    m1.Adr = 0;
    st = UnEnReadData(m1,CardSnr);
	if(st)
	{
		quit(m1.icdev);
		return st;
	}
	//*_CardSnr = CardSnr;
	st = rf_beep(m1.icdev, 5);
    quit(m1.icdev);
    return 0;  
}

