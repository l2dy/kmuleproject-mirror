//this file is part of NeoMule
//Copyright (C)2013 David Xanatos ( XanatosDavid (a) gmail.com / http://NeoLoader.to )
//
#pragma once

#include <string>

class CAddress
{
  public:
    enum EAF
    {
        None = 0,
        IPv4,
        IPv6
    };

    CAddress(EAF eAF = None);
    explicit CAddress(const byte* IP);
    explicit CAddress(UINT IP); // must be same as with Qt, must be in host order
    virtual ~CAddress();

    bool operator < (const CAddress &Other) const;
    bool operator > (const CAddress &Other) const;
    bool operator == (const CAddress &Other) const;
    bool operator != (const CAddress &Other) const;

    void			Init();
    bool			FromString(const std::string Str);
    std::wstring	ToStringW() const;
    UINT			ToIPv4() const; // must be same as with Qt, must be in host order

    bool			IsNull() const;

    bool			ConvertTo(const EAF eAF);

    void			FromSA(const sockaddr* sa, const int sa_len, uint16* pPort = NULL) ;
    void			ToSA(sockaddr* sa, int *sa_len, const uint16 uPort = 0) const;

    int				GetAF() const;
    EAF				GetType() const;
    const unsigned char* Data() const;
    size_t			GetSize() const;

  private:
    std::string		ToString() const;

  protected:
    byte			m_IP[16];
    EAF				m_eAF;
};

char* _inet_ntop(int af, const void *src, char *dst, int size);
int _inet_pton(int af, const char *src, void *dst);

#if 1
#define _ntohl	ntohl
#define _ntohs	ntohs
#else
__inline UINT _ntohl(UINT IP)
{
    UINT PI = 0;

    ((byte*)&PI)[0] = ((byte*)&IP)[3];
    ((byte*)&PI)[1] = ((byte*)&IP)[2];
    ((byte*)&PI)[2] = ((byte*)&IP)[1];
    ((byte*)&PI)[3] = ((byte*)&IP)[0];

    return PI;
}

__inline uint16 _ntohs(uint16 PT)
{
    uint16 TP = 0;

    ((byte*)&TP)[0] = ((byte*)&PT)[1];
    ((byte*)&TP)[1] = ((byte*)&PT)[0];

    return TP;
}
#endif