//this file is part of NeoMule
//Copyright (C)2013 David Xanatos ( XanatosDavid (a) gmail.com / http://NeoLoader.to )
//

#include "stdafx.h"
#ifdef IPV6_SUPPORT
#include "Address.h"

#ifndef WIN32
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#define SOCKET_ERROR (-1)
#define closesocket close
#define WSAGetLastError() errno
#define SOCKET int
#define INVALID_SOCKET (-1)
#ifdef __APPLE__
#include "errno.h"
#endif
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#define EAFNOSUPPORT    102
#endif

CAddress::CAddress(EAF eAF)
{
	Init();
    m_eAF = eAF;    
}

CAddress::CAddress(const byte* IP)
{
	Init();
    m_eAF = IPv6;
    memcpy(m_IP, IP, GetSize());
}

CAddress::CAddress(UINT IP) // must be same as with Qt
{
	Init();
    m_eAF = IPv4;
    IP = _ntohl(IP);
    memcpy(m_IP, &IP, GetSize());
}

CAddress::~CAddress()
{
}

bool CAddress::operator < (const CAddress &Other) const
{
	if (m_eAF != Other.m_eAF) 
		return m_eAF < Other.m_eAF;
	return memcmp(m_IP, Other.m_IP, GetSize()) < 0;
}

bool CAddress::operator > (const CAddress &Other) const
{
	if (m_eAF != Other.m_eAF) 
		return m_eAF > Other.m_eAF;
	return memcmp(m_IP, Other.m_IP, GetSize()) > 0;
}

bool CAddress::operator == (const CAddress &Other) const	
{
	return (m_eAF == Other.m_eAF) && memcmp(m_IP, Other.m_IP, GetSize()) == 0;
}

bool CAddress::operator != (const CAddress &Other) const	
{
	return !(*this == Other);
}

void CAddress::Init()
{
	m_eAF = None;
	memset(m_IP, 0, 16);
}


UINT CAddress::ToIPv4() const // must be same as with Qt*/
{
    UINT ip = 0;
    if (m_eAF == IPv4)
        memcpy(&ip, m_IP, GetSize());
    return _ntohl(ip);
}

size_t CAddress::GetSize() const
{
	switch (m_eAF)
	{
		case IPv4:	return 4; 
		case IPv6:	return 16; 
		default:	return 0; 
	}
}

int CAddress::GetAF() const
{
    return m_eAF == IPv6 ? AF_INET6 : AF_INET;
}

CAddress::EAF	CAddress::GetType() const
{
	return m_eAF;
}

const unsigned char* CAddress::Data() const
{
	return m_IP;
}

std::string CAddress::ToString() const
{
    char Dest[65] = {'\0'};
    if (m_eAF != None)
        _inet_ntop(GetAF(), m_IP, Dest, sizeof(Dest));
    return Dest;
}

std::wstring CAddress::ToStringW() const 
{
	std::string s = ToString(); 
	return std::wstring(s.begin(), s.end());
}

bool CAddress::FromString(const std::string Str)
{
    if (Str.find(".") != std::string::npos)
        m_eAF = IPv4;
    else if (Str.find(":") != std::string::npos)
        m_eAF = IPv6;
    else
	{
		ASSERT(0);
        return false;
	}
    return _inet_pton(GetAF(), Str.c_str(), m_IP) == 1;
}

void CAddress::FromSA(const sockaddr* sa, const int sa_len, uint16* pPort)
{
    switch (sa->sa_family)
    {
        case AF_INET:
        {
            ASSERT(sizeof(sockaddr_in) == sa_len);
            sockaddr_in* sa4 = (sockaddr_in*)sa;

            m_eAF = IPv4;
            *((UINT*)m_IP) = sa4->sin_addr.s_addr;
            if (pPort)
                *pPort = _ntohs(sa4->sin_port);
            break;
        }

        case AF_INET6:
        {
            ASSERT(sizeof(sockaddr_in6) == sa_len);
            sockaddr_in6* sa6 = (sockaddr_in6*)sa;

            m_eAF = IPv6;
            memcpy(m_IP, &sa6->sin6_addr, 16);
            if (pPort)
                *pPort  = _ntohs(sa6->sin6_port);
            break;
        }

        default:
        {
            //WiZaRd: happens e.g. when a disconnect occurs and we don't have the IP, yet
            ASSERT(0);
            m_eAF = None;
            break;
        }
    }
}

void CAddress::ToSA(sockaddr* sa, int *sa_len, const uint16 uPort) const
{
    switch (m_eAF)
    {
        case IPv4:
        {
            ASSERT(sizeof(sockaddr_in) <= *sa_len);
            sockaddr_in* sa4 = (sockaddr_in*)sa;
            *sa_len = sizeof(sockaddr_in);
            memset(sa, 0, *sa_len);

            sa4->sin_family = AF_INET;
            sa4->sin_addr.s_addr = *((UINT*)m_IP);
            sa4->sin_port = htons(uPort);
            break;
        }

        case IPv6:
        {
            ASSERT(sizeof(sockaddr_in6) <= *sa_len);
            sockaddr_in6* sa6 = (sockaddr_in6*)sa;
            *sa_len = sizeof(sockaddr_in6);
            memset(sa, 0, *sa_len);

            sa6->sin6_family = AF_INET6;
            memcpy(&sa6->sin6_addr, m_IP, 16);
            sa6->sin6_port = htons(uPort);
            break;
        }

        default:
            ASSERT(0);
    }
}

bool CAddress::IsNull() const
{
    switch (m_eAF)
    {
        case None:	return true;
        case IPv4:	return *((UINT*)m_IP) == INADDR_ANY;
        case IPv6:	return ((uint64*)m_IP)[0] == 0 && ((uint64*)m_IP)[1] == 0;
    }
    return true;
}

bool CAddress::ConvertTo(const EAF eAF)
{
	bool bConverted = true;
    if (eAF != m_eAF)
	{
		if (eAF == IPv6)
		{
			m_IP[12] = m_IP[0];
			m_IP[13] = m_IP[1];
			m_IP[14] = m_IP[2];
			m_IP[15] = m_IP[3];

			m_IP[10] = m_IP[11] = 0xFF;
			m_IP[0] = m_IP[1] = m_IP[2] = m_IP[3] = m_IP[4] = m_IP[5] = m_IP[6] = m_IP[7] = m_IP[8] = m_IP[9] = 0;

			m_eAF = eAF;
		}
		else if (m_IP[10] == 0xFF && m_IP[11] == 0xFF
				 && !m_IP[0] && !m_IP[1] && !m_IP[2] && !m_IP[3] && !m_IP[4] && !m_IP[5] && !m_IP[6] && !m_IP[7] && !m_IP[8] && !m_IP[9])
		{
			m_IP[0] = m_IP[12];
			m_IP[1] = m_IP[13];
			m_IP[2] = m_IP[14];
			m_IP[3] = m_IP[15];

			m_eAF = eAF;
		}
		else
			bConverted = false;
	}    
    return bConverted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// from BSD sources: http://code.google.com/p/plan9front/source/browse/sys/src/ape/lib/bsd/?r=320990f52487ae84e28961517a4fa0d02d473bac

char* _inet_ntop(int af, const void *src, char *dst, int size)
{
    unsigned char *p;
    char *t;
    int i;

    if (af == AF_INET)
    {
        if (size < INET_ADDRSTRLEN)
        {
            errno = ENOSPC;
            return NULL;
        }
        p = (unsigned char*)&(((struct in_addr*)src)->s_addr);
        sprintf(dst, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        return dst;
    }

    if (af != AF_INET6)
    {
        errno = EAFNOSUPPORT;
        return NULL;
    }

    if (size < INET6_ADDRSTRLEN)
    {
        errno = ENOSPC;
        return NULL;
    }

    p = (unsigned char*)((struct in6_addr*)src)->s6_addr;
    t = dst;
    for (i=0; i<16; i += 2)
    {
        unsigned int w;

        if (i > 0)
            *t++ = ':';
        w = p[i]<<8 | p[i+1];
        sprintf(t, "%x", w);
        t += strlen(t);
    }
    return dst;
}

//////////////////////////////////////////////////////////////////////////////////////

#define CLASS(x)        (x[0]>>6)

int _inet_aton(const char *from, struct in_addr *in)
{
    unsigned char *to;
    unsigned long x;
    char *p;
    int i;

    in->s_addr = 0;
    to = (unsigned char*)&in->s_addr;
    if (*from == 0)
        return 0;
    for (i = 0; i < 4 && *from; i++, from = p)
    {
        x = strtoul(from, &p, 0);
        if (x != (unsigned char)x || p == from)
            return 0;       /* parse error */
        to[i] = (unsigned char)x;
        if (*p == '.')
            p++;
        else if (*p != 0)
            return 0;       /* parse error */
    }

    switch (CLASS(to))
    {
        case 0: /* class A - 1 byte net */
        case 1:
            if (i == 3)
            {
                to[3] = to[2];
                to[2] = to[1];
                to[1] = 0;
            }
            else if (i == 2)
            {
                to[3] = to[1];
                to[1] = 0;
            }
            break;
        case 2: /* class B - 2 byte net */
            if (i == 3)
            {
                to[3] = to[2];
                to[2] = 0;
            }
            break;
    }
    return 1;
}


static int ipcharok(int c)
{
    return c == ':' || isascii(c) && isxdigit(c);
}

static int delimchar(int c)
{
    if (c == '\0')
        return 1;
    if (c == ':' || isascii(c) && isalnum(c))
        return 0;
    return 1;
}

int _inet_pton(int af, const char *src, void *dst)
{
    int i, elipsis = 0;
    unsigned char *to;
    unsigned long x;
    const char *p, *op;

    if (af == AF_INET)
        return _inet_aton(src, (struct in_addr*)dst);

    if (af != AF_INET6)
    {
        errno = EAFNOSUPPORT;
        return -1;
    }

    to = ((struct in6_addr*)dst)->s6_addr;
    memset(to, 0, 16);

    p = src;
    for (i = 0; i < 16 && ipcharok(*p); i+=2)
    {
        op = p;
        x = strtoul(p, (char**)&p, 16);

        if (x != (unsigned short)x || *p != ':' && !delimchar(*p))
            return 0;                       /* parse error */

        to[i] = (unsigned char)(x>>8);
        to[i+1] = (unsigned char)x;
        if (*p == ':')
        {
            if (*++p == ':')        /* :: is elided zero short(s) */
            {
                if (elipsis)
                    return 0;       /* second :: */
                elipsis = i+2;
                p++;
            }
        }
        else if (p == op)               /* strtoul made no progress? */
            break;
    }
    if (p == src || !delimchar(*p))
        return 0;                               /* parse error */
    if (i < 16)
    {
        memmove(&to[elipsis+16-i], &to[elipsis], i-elipsis);
        memset(&to[elipsis], 0, 16-i);
    }
    return 1;
}
#endif