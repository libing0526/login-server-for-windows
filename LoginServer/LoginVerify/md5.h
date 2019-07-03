///*
//**********************************************************************
//** md5.h -- Header file for implementation of MD5                   **
//** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
//** Created: 2/17/90 RLR                                             **
//** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version              **
//** Revised (for MD5): RLR 4/27/91                                   **
//**   -- G modified to have y&~z instead of y&z                      **
//**   -- FF, GG, HH modified to add in last register done            **
//**   -- Access pattern: round 2 works mod 5, round 3 works mod 3    **
//**   -- distinct additive constant for each step                    **
//**   -- round 4 added, working mod 7                                **
//**********************************************************************
//*/
//
///*
//**********************************************************************
//** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
//**                                                                  **
//** License to copy and use this software is granted provided that   **
//** it is identified as the "RSA Data Security, Inc. MD5 Message     **
//** Digest Algorithm" in all material mentioning or referencing this **
//** software or this function.                                       **
//**                                                                  **
//** License is also granted to make and use derivative works         **
//** provided that such works are identified as "derived from the RSA **
//** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
//** material mentioning or referencing the derived work.             **
//**                                                                  **
//** RSA Data Security, Inc. makes no representations concerning      **
//** either the merchantability of this software or the suitability   **
//** of this software for any particular purpose.  It is provided "as **
//** is" without express or implied warranty of any kind.             **
//**                                                                  **
//** These notices must be retained in any copies of any part of this **
//** documentation and/or software.                                   **
//**********************************************************************
//*/
//
///* typedef a 32 bit type */
//typedef unsigned long int UINT4;
//
///* Data structure for MD5 (Message Digest) computation */
//typedef struct {
//    UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
//    UINT4 buf[4];                                    /* scratch buffer */
//    unsigned char in[64];                              /* input buffer */
//    unsigned char digest[16];     /* actual digest after MD5Final call */
//} MD5_CTX;
//
//void MD5Init(MD5_CTX *mdContext);
//void MD5Update(MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
//void MD5Final(MD5_CTX *mdContext);
//
///*
//**********************************************************************
//** End of md5.h                                                     **
//******************************* (cut) ********************************
//*/



//new 
/*
*******************************************************
* brief: md5 encryption
* author: Monkey.Knight
*******************************************************
*/

#ifndef __MD5_ENCODE_H__
#define __MD5_ENCODE_H__

// std
#include <string>

// define
#define UInt32 unsigned int
#define BIT_OF_BYTE  8
#define BIT_OF_GROUP 512
#define SRC_DATA_LEN 64

// 四个非线性函数宏定义
#define DEF_F(X, Y, Z ) ((( (X) & (Y) )|((~X)&(Z))))
#define DEF_G(X, Y, Z)  (((X)&(Z))|((Y)&(~Z)))
#define DEF_H(X, Y, Z)  ((X)^(Y)^(Z))
#define DEF_I(X, Y, Z)  ((Y)^((X)|(~Z)))

// 求链接数函数宏定义
#define FF(a, b, c, d, Mj, s, ti)  (a = b + CycleMoveLeft((a + DEF_F(b,c,d) + Mj + ti),s));
#define GG(a, b, c, d, Mj, s, ti)  (a = b + CycleMoveLeft((a + DEF_G(b,c,d) + Mj + ti),s));
#define HH(a, b, c, d, Mj, s, ti)  (a = b + CycleMoveLeft((a + DEF_H(b,c,d) + Mj + ti),s));
#define II(a, b, c, d, Mj, s, ti)  (a = b + CycleMoveLeft((a + DEF_I(b,c,d) + Mj + ti),s));


class Md5Encode {
public:
    // 4轮循环算法
    struct ParamDynamic{
        UInt32 ua_;
        UInt32 ub_;
        UInt32 uc_;
        UInt32 ud_;
        UInt32 va_last_;
        UInt32 vb_last_;
        UInt32 vc_last_;
        UInt32 vd_last_;
    };
public:
    Md5Encode() {
    }
    std::string Encode(std::string src_info);

protected:

    UInt32 CycleMoveLeft(UInt32 src_num, int bit_num_to_move);
    UInt32 FillData(const char *in_data_ptr, int data_byte_len, char** out_data_ptr);
    void RoundF(char *data_512_ptr, ParamDynamic & param);
    void RoundG(char *data_512_ptr, ParamDynamic & param);
    void RoundH(char *data_512_ptr, ParamDynamic & param);
    void RoundI(char *data_512_ptr, ParamDynamic & param);
    void RotationCalculate(char *data_512_ptr, ParamDynamic & param);
    std::string GetHexStr(unsigned int num_str);

private:
    // 幻数定义
    static const int kA;
    static const int kB;
    static const int kC;
    static const int kD;
    static const unsigned long long k_ti_num_integer;
};

#endif