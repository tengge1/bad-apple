#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib")

#if 0

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#pragma pack(push, 1)

struct TGAHeader
{
	uint8	m_IDLength;
	uint8	m_ColorMapType;
	uint8	m_ImageType;
	uint16	m_CMapStart;
	uint16	m_CMapLength;
	uint8	m_CMapDepth;
	uint16	m_XOffset;
	uint16	m_YOffset;
	uint16	m_Width;
	uint16	m_Height;
	uint8	m_PixelDepth;
	uint8	m_ImageDescriptor;
};

#pragma pack(pop)


bool TgaToTxt( char *pTgaFile, char *pTextFile, int frame )
{
	FILE		*fp, *fo;
	TGAHeader	hdr;
	int			size;
	char		*buf;
	bool		res;

	fp = NULL;
	fo = NULL;
	buf = NULL;
	res = NULL;

	fp = fopen( pTgaFile, "rb" );

	if ( !fp )
		goto err;

	if ( fread( &hdr, sizeof( TGAHeader ), 1, fp ) != 1 )
		goto err;

	if ( hdr.m_ImageType != 2 )
		goto err;

	if ( hdr.m_PixelDepth != 24 )
		goto err;

	size = ( hdr.m_Width + 1 ) * hdr.m_Height;

	buf = new char[ size + 1 ];

	for ( int y = 0; y < hdr.m_Height; y++ )
	{
		for ( int x = 0; x < hdr.m_Width; x++ )
		{
			uint8 rgb[ 3 ];
			char *pixel = &buf[ ( y * ( hdr.m_Width + 1 ) )  + x ];

			if ( fread( &rgb, sizeof( rgb ), 1, fp ) != 1 )
				goto err;

			if ( rgb[ 0 ] > 128 || rgb[ 1 ] > 128 || rgb[ 2 ] > 128 )
				*pixel = '#';
			else
				*pixel = ' ';

			if ( x == hdr.m_Width - 1 )	// The last pixel of line, append LF char
				*( pixel + 1 ) = '\n';
		}
	}

	buf[ size ] = '\0';

	fo = fopen( pTextFile, "ab" );

	if ( !fo )
		goto err;

	fprintf( fo, "$%04d\n", frame );

	fwrite( buf, size - 1, 1, fo );

	fprintf( fo, "\n" );

	res = true;

err:
	if ( fp )
		fclose( fp );

	if ( fo )
		fclose( fo );

	if ( buf )
		delete[] buf;

	return res;
}

#endif


#define DATAFILE "BADAPPLE.data"

#define FRAME_WIDTH		80
#define FRAME_HEIGHT	32

#define DATA_TOKEN_SIZE	( 5 + 1 )	//	$0000\n
#define DATA_PITCH_SIZE	( FRAME_WIDTH + 1 )		// ###...\n
#define DATA_FRAME_SIZE	( DATA_TOKEN_SIZE + ( DATA_PITCH_SIZE * FRAME_HEIGHT ) )

#define FRAME_COUNT	3271

#define VIDEO_TIME	218000.0f	// 3:38 (218000ms)


void main(void)
{
	HANDLE hOutput;

	hOutput = GetStdHandle( STD_OUTPUT_HANDLE );

	CONSOLE_CURSOR_INFO coc = { 1, FALSE };
	COORD co = { FRAME_WIDTH, FRAME_HEIGHT };
	SMALL_RECT rc = { 0, 0, FRAME_WIDTH - 1, FRAME_HEIGHT - 1 };

	SetConsoleCursorInfo( hOutput, &coc );
	SetConsoleScreenBufferSize( hOutput, co );
	SetConsoleWindowInfo( hOutput, TRUE, &rc );

	SetConsoleTitle( "Bad Apple By Crsky @2016" );

#if 0
	for ( int i = 0; i < 3271; i++ )
	{
		char file_tga[ MAX_PATH ];
		sprintf( file_tga, "Sequence\\BADAPPLE%04d.tga", i );
		TgaToTxt( file_tga, "BADAPPLE.txt", i );
	}
#endif

	FILE	*fp;
	int		size;
	char	*buf;
	char	*cur;
	int		start;

	buf = NULL;

	fp = fopen( DATAFILE, "rb" );

	if ( !fp )
	{
		printf( "%s not found.\n", DATAFILE );
		goto err;
	}

	size = DATA_FRAME_SIZE * FRAME_COUNT;	// Data size per frame (2598)

	buf = new char[ size ];

	if ( fread( buf, size - 1, 1, fp ) != 1 )
	{
		printf( "Failed to read in data file.\n" );
		goto err;
	}

	buf[ size - 1 ] = '\0';

	fclose( fp );

	mciSendString( "open BADAPPLE.wma alias BGM", NULL, 0, NULL );
	mciSendString( "play BGM", NULL, 0, NULL );

	start = GetTickCount();

	while ( 1 )
	{
		int		time;
		float	percen;
		int		frame;
		COORD	xy = { 0, 0 };
		DWORD	written;

		if ( GetKeyState( VK_SPACE ) )
		{
			printf( "Stop play.\n" );
			break;
		}

		time = GetTickCount();
		percen = ( time - start ) / VIDEO_TIME;

		if ( percen > 1 )
		{
			printf( "End of play.\n" );
			break;
		}

		frame = (int)(percen * FRAME_COUNT);
		cur = &buf[ ( DATA_FRAME_SIZE * frame ) + DATA_TOKEN_SIZE ];

		for ( ; xy.Y < FRAME_HEIGHT; xy.Y++, cur += DATA_PITCH_SIZE )
			WriteConsoleOutputCharacter( hOutput, cur, DATA_PITCH_SIZE - 1, xy, &written );

		Sleep( 60 );
	}

	mciSendString( "stop", NULL, 0, NULL );
	mciSendString( "close", NULL, 0, NULL );

err:
	if ( buf )
		delete[] buf;

	Sleep( 500 );
}