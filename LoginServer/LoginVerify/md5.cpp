//#include "md5.h"
// 
///*
//**********************************************************************
//** md5.c                                                            **
//** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
//** Created: 2/17/90 RLR                                             **
//** Revised: 1/91 SRD,AJ,BSK,JT Reference C Version                  **
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
///* -- include the following line if the md5.h header file is separate -- */
///* #include "md5.h" */
// 
///* forward declaration */
//static void Transform();
//static void TransformMD5(UINT4 *buf, UINT4 *in);
// 
//static unsigned char PADDING[64] = {
//	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
//};
// 
///* F, G and H are basic MD5 functions: selection, majority, parity */
//#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
//#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
//#define H(x, y, z) ((x) ^ (y) ^ (z))
//#define I(x, y, z) ((y) ^ ((x) | (~z))) 
// 
///* ROTATE_LEFT rotates x left n bits */
//#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
// 
///* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
///* Rotation is separate from addition to prevent recomputation */
//#define FF(a, b, c, d, x, s, ac) \
//  {(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
//   (a) = ROTATE_LEFT ((a), (s)); \
//   (a) += (b); \
//  }
//#define GG(a, b, c, d, x, s, ac) \
//	  {(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
//   (a) = ROTATE_LEFT ((a), (s)); \
//   (a) += (b); \
//	  }
//#define HH(a, b, c, d, x, s, ac) \
//	  {(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
//   (a) = ROTATE_LEFT ((a), (s)); \
//   (a) += (b); \
//	  }
//#define II(a, b, c, d, x, s, ac) \
//	  {(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
//   (a) = ROTATE_LEFT ((a), (s)); \
//   (a) += (b); \
//	  }
// 
//void MD5Init(MD5_CTX *mdContext)
//{
//	mdContext->i[0] = mdContext->i[1] = (UINT4)0;
// 
//	/* Load magic initialization constants.
//	*/
//	mdContext->buf[0] = (UINT4)0x67452301;
//	mdContext->buf[1] = (UINT4)0xefcdab89;
//	mdContext->buf[2] = (UINT4)0x98badcfe;
//	mdContext->buf[3] = (UINT4)0x10325476;
//}
// 
//void MD5Update(MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
//{
//	UINT4 in[16];
//	int mdi;
//	unsigned int i, ii;
// 
//	/* compute number of bytes mod 64 */
//	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);
// 
//	/* update number of bits */
//	if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
//		mdContext->i[1]++;
//	mdContext->i[0] += ((UINT4)inLen << 3);
//	mdContext->i[1] += ((UINT4)inLen >> 29);
// 
//	while (inLen--) {
//		/* add new character to buffer, increment mdi */
//		mdContext->in[mdi++] = *inBuf++;
// 
//		/* transform if necessary */
//		if (mdi == 0x40) {
//			for (i = 0, ii = 0; i < 16; i++, ii += 4)
//				in[i] = (((UINT4)mdContext->in[ii + 3]) << 24) |
//				(((UINT4)mdContext->in[ii + 2]) << 16) |
//				(((UINT4)mdContext->in[ii + 1]) << 8) |
//				((UINT4)mdContext->in[ii]);
//			TransformMD5(mdContext->buf, in);
//			mdi = 0;
//		}
//	}
//}
// 
//void MD5Final(MD5_CTX *mdContext)
// 
//{
//	UINT4 in[16];
//	int mdi;
//	unsigned int i, ii;
//	unsigned int padLen;
// 
//	/* save number of bits */
//	in[14] = mdContext->i[0];
//	in[15] = mdContext->i[1];
// 
//	/* compute number of bytes mod 64 */
//	mdi = (int)((mdContext->i[0] >> 3) & 0x3F);
// 
//	/* pad out to 56 mod 64 */
//	padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
//	MD5Update(mdContext, PADDING, padLen);
// 
//	/* append length in bits and transform */
//	for (i = 0, ii = 0; i < 14; i++, ii += 4)
//		in[i] = (((UINT4)mdContext->in[ii + 3]) << 24) |
//		(((UINT4)mdContext->in[ii + 2]) << 16) |
//		(((UINT4)mdContext->in[ii + 1]) << 8) |
//		((UINT4)mdContext->in[ii]);
//	TransformMD5(mdContext->buf, in);
// 
//	/* store buffer in digest */
//	for (i = 0, ii = 0; i < 4; i++, ii += 4) {
//		mdContext->digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
//		mdContext->digest[ii + 1] =
//			(unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
//		mdContext->digest[ii + 2] =
//			(unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
//		mdContext->digest[ii + 3] =
//			(unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
//	}
//}
// 
///* Basic MD5 step. Transform buf based on in.
//*/
//static void TransformMD5(UINT4 *buf, UINT4 *in)
//{
//	UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];
// 
//	/* Round 1 */
//#define S11 7
//#define S12 12
//#define S13 17
//#define S14 22
//	FF(a, b, c, d, in[0], S11, 3614090360); /* 1 */
//	FF(d, a, b, c, in[1], S12, 3905402710); /* 2 */
//	FF(c, d, a, b, in[2], S13, 606105819); /* 3 */
//	FF(b, c, d, a, in[3], S14, 3250441966); /* 4 */
//	FF(a, b, c, d, in[4], S11, 4118548399); /* 5 */
//	FF(d, a, b, c, in[5], S12, 1200080426); /* 6 */
//	FF(c, d, a, b, in[6], S13, 2821735955); /* 7 */
//	FF(b, c, d, a, in[7], S14, 4249261313); /* 8 */
//	FF(a, b, c, d, in[8], S11, 1770035416); /* 9 */
//	FF(d, a, b, c, in[9], S12, 2336552879); /* 10 */
//	FF(c, d, a, b, in[10], S13, 4294925233); /* 11 */
//	FF(b, c, d, a, in[11], S14, 2304563134); /* 12 */
//	FF(a, b, c, d, in[12], S11, 1804603682); /* 13 */
//	FF(d, a, b, c, in[13], S12, 4254626195); /* 14 */
//	FF(c, d, a, b, in[14], S13, 2792965006); /* 15 */
//	FF(b, c, d, a, in[15], S14, 1236535329); /* 16 */
// 
//	/* Round 2 */
//#define S21 5
//#define S22 9
//#define S23 14
//#define S24 20
//	GG(a, b, c, d, in[1], S21, 4129170786); /* 17 */
//	GG(d, a, b, c, in[6], S22, 3225465664); /* 18 */
//	GG(c, d, a, b, in[11], S23, 643717713); /* 19 */
//	GG(b, c, d, a, in[0], S24, 3921069994); /* 20 */
//	GG(a, b, c, d, in[5], S21, 3593408605); /* 21 */
//	GG(d, a, b, c, in[10], S22, 38016083); /* 22 */
//	GG(c, d, a, b, in[15], S23, 3634488961); /* 23 */
//	GG(b, c, d, a, in[4], S24, 3889429448); /* 24 */
//	GG(a, b, c, d, in[9], S21, 568446438); /* 25 */
//	GG(d, a, b, c, in[14], S22, 3275163606); /* 26 */
//	GG(c, d, a, b, in[3], S23, 4107603335); /* 27 */
//	GG(b, c, d, a, in[8], S24, 1163531501); /* 28 */
//	GG(a, b, c, d, in[13], S21, 2850285829); /* 29 */
//	GG(d, a, b, c, in[2], S22, 4243563512); /* 30 */
//	GG(c, d, a, b, in[7], S23, 1735328473); /* 31 */
//	GG(b, c, d, a, in[12], S24, 2368359562); /* 32 */
// 
//	/* Round 3 */
//#define S31 4
//#define S32 11
//#define S33 16
//#define S34 23
//	HH(a, b, c, d, in[5], S31, 4294588738); /* 33 */
//	HH(d, a, b, c, in[8], S32, 2272392833); /* 34 */
//	HH(c, d, a, b, in[11], S33, 1839030562); /* 35 */
//	HH(b, c, d, a, in[14], S34, 4259657740); /* 36 */
//	HH(a, b, c, d, in[1], S31, 2763975236); /* 37 */
//	HH(d, a, b, c, in[4], S32, 1272893353); /* 38 */
//	HH(c, d, a, b, in[7], S33, 4139469664); /* 39 */
//	HH(b, c, d, a, in[10], S34, 3200236656); /* 40 */
//	HH(a, b, c, d, in[13], S31, 681279174); /* 41 */
//	HH(d, a, b, c, in[0], S32, 3936430074); /* 42 */
//	HH(c, d, a, b, in[3], S33, 3572445317); /* 43 */
//	HH(b, c, d, a, in[6], S34, 76029189); /* 44 */
//	HH(a, b, c, d, in[9], S31, 3654602809); /* 45 */
//	HH(d, a, b, c, in[12], S32, 3873151461); /* 46 */
//	HH(c, d, a, b, in[15], S33, 530742520); /* 47 */
//	HH(b, c, d, a, in[2], S34, 3299628645); /* 48 */
// 
//	/* Round 4 */
//#define S41 6
//#define S42 10
//#define S43 15
//#define S44 21
//	II(a, b, c, d, in[0], S41, 4096336452); /* 49 */
//	II(d, a, b, c, in[7], S42, 1126891415); /* 50 */
//	II(c, d, a, b, in[14], S43, 2878612391); /* 51 */
//	II(b, c, d, a, in[5], S44, 4237533241); /* 52 */
//	II(a, b, c, d, in[12], S41, 1700485571); /* 53 */
//	II(d, a, b, c, in[3], S42, 2399980690); /* 54 */
//	II(c, d, a, b, in[10], S43, 4293915773); /* 55 */
//	II(b, c, d, a, in[1], S44, 2240044497); /* 56 */
//	II(a, b, c, d, in[8], S41, 1873313359); /* 57 */
//	II(d, a, b, c, in[15], S42, 4264355552); /* 58 */
//	II(c, d, a, b, in[6], S43, 2734768916); /* 59 */
//	II(b, c, d, a, in[13], S44, 1309151649); /* 60 */
//	II(a, b, c, d, in[4], S41, 4149444226); /* 61 */
//	II(d, a, b, c, in[11], S42, 3174756917); /* 62 */
//	II(c, d, a, b, in[2], S43, 718787259); /* 63 */
//	II(b, c, d, a, in[9], S44, 3951481745); /* 64 */
// 
//	buf[0] += a;
//	buf[1] += b;
//	buf[2] += c;
//	buf[3] += d;
//}
// 
///*
//**********************************************************************
//** End of md5.c                                                     **
//******************************* (cut) ********************************
//*/



//new 

#include "md5.h"
#include <iostream>
#include <cmath>
using namespace std;
//using namespace math;
// 幻数定义
const int Md5Encode::kA = 0x67452301;
const int Md5Encode::kB = 0xefcdab89;
const int Md5Encode::kC = 0x98badcfe;
const int Md5Encode::kD = 0x10325476;

const unsigned long long Md5Encode::k_ti_num_integer = 4294967296;

// function: CycleMoveLeft
// @param src_num:要左移的数
// @param bit_num_to_move:要移动的bit位数
// @return  循环左移后的结果数
UInt32 Md5Encode::CycleMoveLeft(UInt32 src_num, int bit_num_to_move) {
    UInt32 src_num1 = src_num;
    UInt32 src_num2 = src_num;
    if (0 >= bit_num_to_move) {
        return src_num;
    }
    UInt32 num1 = src_num1 << bit_num_to_move;
    UInt32 num2 = src_num2 >> (32 - bit_num_to_move);

    return ((src_num1 << bit_num_to_move) \
        | (src_num2 >> (32 - bit_num_to_move)));
}

// function: FillData
// @param in_data_ptr:    要加密的信息数据
// @param data_byte_len: 数据的字节数
// @param out_data_ptr:  填充必要信息后的数据
// return : 填充信息后的数据长度,以字节为单位
UInt32 Md5Encode::FillData(const char *in_data_ptr, int data_byte_len, char** out_data_ptr) {
    int bit_num = data_byte_len*BIT_OF_BYTE;
    int grop_num = bit_num / BIT_OF_GROUP;
    int mod_bit_num = bit_num % BIT_OF_GROUP;
    int bit_need_fill = 0;
    if (mod_bit_num > (BIT_OF_GROUP - SRC_DATA_LEN)) {
        bit_need_fill = (BIT_OF_GROUP - mod_bit_num);
        bit_need_fill += (BIT_OF_GROUP - SRC_DATA_LEN);
    }
    else {
        bit_need_fill = (BIT_OF_GROUP - SRC_DATA_LEN) - mod_bit_num; //  这里多加了一个BIT_OF_GROUP，避免bit_need_fill正好等于0,暂时不加
    }
    int all_bit = bit_num + bit_need_fill;
    if (0 < bit_need_fill) {
        *out_data_ptr = new char[all_bit / BIT_OF_BYTE + SRC_DATA_LEN / BIT_OF_BYTE];
        memset(*out_data_ptr, 0, all_bit / BIT_OF_BYTE + SRC_DATA_LEN / BIT_OF_BYTE);
        // copy data
        memcpy(*out_data_ptr, in_data_ptr, data_byte_len);
        // fill rest data
        unsigned char *tmp = reinterpret_cast<unsigned char *>(*out_data_ptr);
        tmp += data_byte_len;
        // fill 1 and 0
        *tmp = 0x80;
        // fill origin data len
        unsigned long long * origin_num = (unsigned long long *)((*out_data_ptr) + ((all_bit / BIT_OF_BYTE)));
        *origin_num = data_byte_len*BIT_OF_BYTE;
    }
    return (all_bit / BIT_OF_BYTE + SRC_DATA_LEN / BIT_OF_BYTE);
}

void Md5Encode::RoundF(char *data_BIT_OF_GROUP_ptr, ParamDynamic & param) {
    UInt32 *M = reinterpret_cast<UInt32*>(data_BIT_OF_GROUP_ptr);
    int s[] = { 7, 12, 17, 22 };
    for (int i = 0; i < 16; ++i) {
        long double tmpLong = i+1;
        UInt32 ti = k_ti_num_integer * abs(sin(tmpLong));
        if (i % 4 == 0) {
            FF(param.ua_, param.ub_, param.uc_, param.ud_, M[i], s[i % 4], ti);
        }
        else if (i % 4 == 1) {
            FF(param.ud_, param.ua_, param.ub_, param.uc_, M[i], s[i % 4], ti);
        }
        else if (i % 4 == 2) {
            FF(param.uc_, param.ud_, param.ua_, param.ub_, M[i], s[i % 4], ti);
        }
        else if (i % 4 == 3) {
            FF(param.ub_, param.uc_, param.ud_, param.ua_, M[i], s[i % 4], ti);
        }
    }
}

void Md5Encode::RoundG(char *data_BIT_OF_GROUP_ptr, ParamDynamic & param) {
    UInt32 *M = reinterpret_cast<UInt32*>(data_BIT_OF_GROUP_ptr);
    int s[] = { 5, 9, 14, 20 };
    for (int i = 0; i < 16; ++i) {
        long double tmpLong = i + 1 + 16;
        UInt32 ti = k_ti_num_integer * abs(sin(tmpLong));
        int index = (i * 5 + 1) % 16;
        if (i % 4 == 0) {
            GG(param.ua_, param.ub_, param.uc_, param.ud_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 1) {
            GG(param.ud_, param.ua_, param.ub_, param.uc_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 2) {
            GG(param.uc_, param.ud_, param.ua_, param.ub_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 3) {
            GG(param.ub_, param.uc_, param.ud_, param.ua_, M[index], s[i % 4], ti);
        }
    }
}

void Md5Encode::RoundH(char *data_BIT_OF_GROUP_ptr, ParamDynamic & param) {
    UInt32 *M = reinterpret_cast<UInt32*>(data_BIT_OF_GROUP_ptr);
    int s[] = { 4, 11, 16, 23 };
    for (int i = 0; i < 16; ++i) {
        long double tmpLong = i + 1 + 32;
        UInt32 ti = k_ti_num_integer * abs(sin(tmpLong));
        int index = (i * 3 + 5) % 16;
        if (i % 4 == 0) {
            HH(param.ua_, param.ub_, param.uc_, param.ud_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 1) {
            HH(param.ud_, param.ua_, param.ub_, param.uc_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 2) {
            HH(param.uc_, param.ud_, param.ua_, param.ub_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 3) {
            HH(param.ub_, param.uc_, param.ud_, param.ua_, M[index], s[i % 4], ti);
        }
    }
}

void Md5Encode::RoundI(char *data_BIT_OF_GROUP_ptr, ParamDynamic & param) {
    UInt32 *M = reinterpret_cast<UInt32*>(data_BIT_OF_GROUP_ptr);
    int s[] = { 6, 10, 15, 21 };
    for (int i = 0; i < 16; ++i) {
        long double tmpLong = i + 1 + 48;
        UInt32 ti = k_ti_num_integer * abs(sin(tmpLong));
        int index = (i * 7 + 0) % 16;
        if (i % 4 == 0) {
            II(param.ua_, param.ub_, param.uc_, param.ud_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 1) {
            II(param.ud_, param.ua_, param.ub_, param.uc_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 2) {
            II(param.uc_, param.ud_, param.ua_, param.ub_, M[index], s[i % 4], ti);
        }
        else if (i % 4 == 3) {
            II(param.ub_, param.uc_, param.ud_, param.ua_, M[index], s[i % 4], ti);
        }
    }
}

void Md5Encode::RotationCalculate(char *data_512_ptr, ParamDynamic & param) {
    if (NULL == data_512_ptr) {
        return;
    }
    RoundF(data_512_ptr, param);
    RoundG(data_512_ptr, param);
    RoundH(data_512_ptr, param);
    RoundI(data_512_ptr, param);
    param.ua_ = param.va_last_ + param.ua_;
    param.ub_ = param.vb_last_ + param.ub_;
    param.uc_ = param.vc_last_ + param.uc_;
    param.ud_ = param.vd_last_ + param.ud_;

    param.va_last_ = param.ua_;
    param.vb_last_ = param.ub_;
    param.vc_last_ = param.uc_;
    param.vd_last_ = param.ud_;
}

// 转换成十六进制字符串输出
std::string Md5Encode::GetHexStr(unsigned int num_str) {
    std::string hexstr = "";
    char szch[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    unsigned char *tmptr = (unsigned char *)&num_str;
    int len = sizeof(num_str);
    // 小端字节序，逆序打印
    for (int i = 0; i < len; i++){
        unsigned char ch = tmptr[i] & 0xF0;
        ch = ch >> 4;
        hexstr.append(1, szch[ch]);
        ch = tmptr[i] & 0x0F;
        hexstr.append(1, szch[ch]);
    }
    return hexstr;
}

// function: Encode
// @param src_info:要加密的信息
// return :加密后的MD5值
std::string Md5Encode::Encode(std::string src_info) {
    ParamDynamic param;
    param.ua_ = kA;
    param.ub_ = kB;
    param.uc_ = kC;
    param.ud_ = kD;
    param.va_last_ = kA;
    param.vb_last_ = kB;
    param.vc_last_ = kC;
    param.vd_last_ = kD;

    std::string result;
    const char *src_data = src_info.c_str();
    char *out_data_ptr = NULL;
    int total_byte = FillData(src_data, strlen(src_data), &out_data_ptr);
    //char * data_BIT_OF_GROUP = out_data_ptr;
    for (int i = 0; i < total_byte / (BIT_OF_GROUP / BIT_OF_BYTE); ++i) {
        char * data_BIT_OF_GROUP = out_data_ptr;
        data_BIT_OF_GROUP += i*(BIT_OF_GROUP / BIT_OF_BYTE);
        RotationCalculate(data_BIT_OF_GROUP, param);
    }
    if (NULL != out_data_ptr) {
        delete[] out_data_ptr, out_data_ptr = NULL;
    }
    result.append(GetHexStr(param.ua_));
    result.append(GetHexStr(param.ub_));
    result.append(GetHexStr(param.uc_));
    result.append(GetHexStr(param.ud_));
    return result;
}