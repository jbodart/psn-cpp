//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
/*
The MIT License (MIT)

Copyright (c) 2019 VYV Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#ifdef _WIN32
    #include <Winsock2.h> // before Windows.h, else Winsock 1 conflict
    #include <Ws2tcpip.h> // needed for ip_mreq definition for multicast
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <time.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#include <string>
#include <cstring>
//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#ifdef _WIN32
class wsa_session
{
  public :

    wsa_session()
    {
        WSAStartup( MAKEWORD( 2 , 2 ) , &data_ ) ;
    }
    ~wsa_session()
    {
        WSACleanup() ;
    }

  private : 

    WSAData data_ ;
} ;
#else
int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }
#endif

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
class udp_socket
{
  public :

	udp_socket( void ) 
	{
		// UDP socket
		socket_ = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP ) ;

		// non blocking socket
		u_long arg = 1 ;
#ifdef _WIN32
		ioctlsocket( socket_ , FIONBIO , &arg ) ;
#else
                int flags = guard(fcntl(socket_, F_GETFL), "could not get file flags");
                guard(fcntl(socket_, F_SETFL, flags | O_NONBLOCK),"could not set file flags");
#endif 
	}

	~udp_socket( void ) 
	{
#ifdef _WIN32
		closesocket( socket_ ) ;
#endif
	}

	bool send_message( const std::string & address , unsigned short port , const ::std::string & message )
    {
        sockaddr_in add ;
        add.sin_family = AF_INET ;
#ifdef _WIN32
        InetPton( AF_INET , address.c_str() , &add.sin_addr.s_addr ) ;
#else
        add.sin_addr.s_addr=inet_addr(address.c_str());
#endif
        add.sin_port = htons( port ) ;
        
		if ( sendto( socket_ , message.c_str() , (int)message.length() , 0 , reinterpret_cast<sockaddr *>( &add ) , sizeof( add ) ) > 0 )
			return true ;

		return false ;
    }

    bool receive_message( ::std::string & message , int max_size = 1500 )
    {
		if ( max_size <= 0 )
			return false ;

		char * buffer = (char *)malloc( max_size ) ;

        int byte_recv = recv( socket_ , buffer , max_size , 0 ) ;

		if ( byte_recv > 0 )
			message = ::std::string( buffer , byte_recv ) ;

		free( buffer ) ;

		return ( byte_recv > 0 ) ;
    }

    bool bind( unsigned short port )
    {
        sockaddr_in add ;
        add.sin_family = AF_INET ;
        add.sin_addr.s_addr = htonl( INADDR_ANY ) ;
        add.sin_port = htons( port ) ;

		if ( ::bind( socket_ , reinterpret_cast<sockaddr *>( &add ) , sizeof( add ) ) ==0 )
			return true ;

		return false ;
    }

	bool enable_send_message_multicast( std::string add_str ) 
	{
		struct in_addr add ;
        if(add_str=="") add.s_addr = INADDR_ANY ;
        else add.s_addr =inet_addr(add_str.c_str());

		int result = setsockopt( socket_ , 
						   IPPROTO_IP , 
						   IP_MULTICAST_IF , 
						   (const char*)&add , 
						   sizeof( add ) ) ;
		return ( result ==0 ) ; 
	}

	bool join_multicast_group( const ::std::string & ip_group ) 
	{
		struct ip_mreq imr; 
       
 #ifdef _WIN32
        InetPton( AF_INET , ip_group.c_str() , &imr.imr_multiaddr.s_addr ) ;
#else
        imr.imr_multiaddr.s_addr=inet_addr(ip_group.c_str());
 #endif

		imr.imr_interface.s_addr = INADDR_ANY ;

		int result = setsockopt( socket_ , 
						   IPPROTO_IP , 
						   IP_ADD_MEMBERSHIP , 
						   (char*) &imr , 
						   sizeof(struct ip_mreq) ) ;
		return ( result ==0 ) ; 
	}

	bool leave_multicast_group( const ::std::string & ip_group ) 
	{
		struct ip_mreq imr;
#ifdef _WIN32        
        InetPton( AF_INET , ip_group.c_str() , &imr.imr_multiaddr.s_addr ) ;
#else
        imr.imr_multiaddr.s_addr=inet_addr(ip_group.c_str());
#endif
		imr.imr_interface.s_addr = INADDR_ANY ;

		int result = setsockopt( socket_ , 
						   IPPROTO_IP , 
						   IP_DROP_MEMBERSHIP ,
						   (char*) &imr , 
						   sizeof(struct ip_mreq) ) ;
		return ( result ==0 ) ; 
	}

  private :

	int		socket_ ; 
} ;

#endif
