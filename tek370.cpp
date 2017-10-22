/* $Id: tek370.cpp 176 2008-05-15 21:43:45Z arteq $
 * vim: fenc=utf-8 ts=2
 * Author: Artur Gracki <arteq@arteq.org>
 * Last change: 2008/05/07 (Wed) 12:03:02
 * Keywords: gpib, tektronix, tek 370, curve tracer, 
 */ 

#include	<iostream>
#include	<fstream>
#include	<string>
#include	<math.h>
#include	<getopt.h>
#include	"gpib/ib.h"

using namespace std;

/* ====================================================================== */

const char ver[] = "0.4";
const int bufor_size = 1024;
char bufor [ bufor_size ];
bool pre = false;

/* ====================================================================== */

int init()
{
	int device;

	cerr << "*** TEK 370 ***" << endl;

//  device = ibdev( board_index, pad, sad, TNONE, send_eoi, eos_mode );
	device = ibdev( 0, 1, 0, TNONE, 1, 0); 
	if( device < 0 )
	{
		cerr << "[EE] GPIB init failed:" << endl;
		cerr << "[EE] " << gpib_error_string( ThreadIberr() ) << endl;
		return -1;
		exit(0);
	}
	else
	{
		cerr << "[II] GPIB init OK!" << endl;
		return device;
	}
}

/* ====================================================================== */

void clear()
{
	int dev = init();
	ibwrt( dev, "TEXt \"\"", 7);
	ibwrt( dev, "CURsor OFF", 10);
	ibwrt( dev, "STPgen NUMber:5", 15);
	ibwrt( dev, "DISPLAY NSTore", 14);
	ibclr( dev );
	ibloc( dev );
	ibonl( dev, 0 );
	cerr << "[II] Urzadzenie wyzerowane" << endl;
}

/* ====================================================================== */

void bufor_clear()
{
	for (int i = 0; i < bufor_size; i++)
		bufor[i] = 0;
}

/* ====================================================================== */

void help()
{
	cout << "\n"
			 << " PROGRAM: Tektronix 370B, ver. " << ver << "\n"
			 << "   AUTOR: Artur Gracki <arteq@arteq.org> \n"
			 << "LICENCJA: GNU GPL \n"
			 << "    DATA: " << __DATE__ << "\n\n"
			 << "  UZYCIE: ./tek370 [-o dane.dat] \n"
			 << "          -o, --output dane.dat - plik, do ktorego zostana zapisane odczytane dane\n"
			 << "          -p, --pre             - dodaje preambule do pliku z danymi\n"
			 << "          -c, --clear           - zeruje charakterograf\n"
			 << "          -h, --help            - pomoc\n";

	cout << endl;
}

/* ====================================================================== */

void parsuj_opcje(int argc, char* const *argv, char **file_output)
{
	char next;
	const char* const opcje_short = "ho:pc";
	const struct option opcje_long[] = {
		{ "help",		0,	NULL,		'h'		},
		{	"output", 1,	NULL,		'o'		},
		{ "clear",  0,	NULL,		'c'		},
		{ "pre",		0,	NULL,		'p'		} };

	do 
	{
		next = getopt_long( argc, argv, opcje_short, opcje_long, NULL);
		if (next == EOF) break;
		switch(next)
		{
			case 'h':		help();
									exit(0);
			case 'o':		*file_output = optarg;
									break;
			case 'p':		pre = true;
									break;
			case 'c':		clear();
									exit(0);
			default:		help();
									exit(0);
			case -1:		break;
		}
	}
	while (next != -1);
}

/* ====================================================================== */

int main( int argc, char *argv[] )
{
	int dev;
	int status;
	float div_h, div_v;
	float offset_y = 0.0;
	float offset_x = 0.0;
	char *plik = "dane.dat";
	ofstream fp;

	/* ====================================================================== */

	if (argc > 1) 
	{
		parsuj_opcje(argc, argv, &plik);
	}

	/* ====================================================================== */
	
	bufor_clear();
	dev = init();
	if (dev < 0) exit(0);

	/* ====================================================================== */

	status = ibwrt( dev, "id?", 3);
	if( status & ERR )
	{
		cerr << "[EE] Id failed" << endl;
	}
	else 
	{
		ibrd( dev, bufor, bufor_size );
	}

	cerr << "[II] " << bufor << endl;

	/* ====================================================================== */
	// Ustawiamy parametry pomiaru
	
	ibwrt( dev, "WINdow 90, 925, 800, 1000", 25);
	ibwrt( dev, "TEXt \"Ustawiam parametry...\"", 28);
	ibwrt( dev, "STPgen NUMber:5", 15);
	ibwrt( dev, "MEAsure SINgle", 14);
	ibwrt( dev, "TEXt \"Robie pomiar...\"", 22);
	ibwrt( dev, "DISPLAY STORE", 13);
	ibwrt( dev, "MEAsure SINgle", 14);

	/* ====================================================================== */
	 
	// Wskaznik do pliku
	fp.open(plik, ios::out);

	// Czytam polaryzacje
	bufor_clear();
	ibwrt( dev, "TEXt \"Czytam polaryzacje...\"", 28);
	ibwrt( dev, "CSPol?", 6);
	ibrd( dev, bufor, bufor_size);
	cerr 	<< "[RR] Polaryzacja: \e[32;1m" << bufor << "\e[0m" << endl;
	fp << "# " << bufor << "\n";

	// Czytam ustawienia VERT/div
	bufor_clear();
	ibwrt( dev, "TEXt \"Czytam VERT/div...\"", 25);
	ibwrt( dev, "VERt?", 5);
	ibrd( dev, bufor, bufor_size);
	cerr 	<< "[RR] Vertical: " << bufor << " -> \e[32;1m";
	fp << "# " << bufor << "\n";

	// Czytam AMPS/div
	{
		char ile[10] = "         ";
		for (int i = 13; i < 23; i++)
			if (bufor[i] != (int)",")	
				ile[i-13] = bufor[i];
			else break;
		div_v = atof(ile);
	}

	// Wyswietlam odczytane AMPS	
	if (div_v < 0.001)
		cerr << div_v * 1000000 << " \e[0m[uA/div]" << endl;
	else if (div_v < 1)
		cerr << div_v * 1000 << " \e[0m[mA/div]" << endl;	
	else 
		cerr << div_v << " \e[0m[A/div]" << endl;

	/* ====================================================================== */

	// Czytam ustawienia HORIZ/div
	bufor_clear();
	ibwrt( dev, "TEXt \"Czytam HORIZ/div...\"", 26);
	ibwrt( dev, "HORiz?", 6);
	ibrd( dev, bufor, bufor_size);
	cerr 	<< "[RR] Horizont: " << bufor << " -> \e[32;1m";
	fp << "# " << bufor << "\n";

	// Czytam VOLTS/div
	{
		char ile[10] = "         ";
		for (int i = 14; i < 24; i++)
			if (bufor[i] != (int)",")	
				ile[i-14] = bufor[i];
			else break;
		div_h = atof(ile);
	}

	// Wyswietlam odczytane VOLTS
	if (div_h < 0.001)
		cerr << div_h * 1000000 << " \e[0m[uV/div]" << endl;
	else	if (div_h <	1)
		cerr << div_h * 1000 << " \e[0m[mV/div]" << endl;
	else 
		cerr << div_h << " \e[0m[V/div]" << endl;

	/* ====================================================================== */
	// Odczytuje preambule
	
	bufor_clear();
	ibwrt( dev, "TEXt \"Czytam preambule...\"", 26);
	ibwrt( dev, "WFMpre?", 7);
	ibrd( dev, bufor, bufor_size);
	cerr 	<< "[II] Czytam preambule: ";
	if (pre) fp << "# " << bufor << "\n";

	// Odczytuje XOFF
	{
		char tmp[12] = "           ";
		for (int i = 196; i < 202; i++)
			tmp[i-196] = bufor[i];
		offset_x = atoi(tmp);
	}
	cout << "XOFF: " << offset_x;

	// Odczytuje YOFF
	{
		char tmp[12] = "           ";
		for (int i = 239; i < 244; i++)
			tmp[i-239] = bufor[i];
		offset_y = atoi(tmp);
	}
	cout << " YOFF: " << offset_y << endl;

	/* ====================================================================== */
	// Wlasciwy pomiar - pobranie danych

	int dane_size = 1024;
	unsigned char dane [ dane_size ]; 
	float dane_dx [ dane_size / 4 ];
	float dane_dy [ dane_size / 4 ];

	for (int i = 0; i < dane_size; i++)
		dane[i] = 0;

	cerr << "[II] Rozpoczynam pobieranie danych..." << endl;

	ibwrt( dev, "TEXt \"Przesylam dane do PC...\"", 30);
	ibwrt( dev, "CURVE?", 6);
	ibrd(dev, dane, dane_size);
	if (ibcnt == dane_size)
		cerr << "[II] Dane pobrane OK" << endl;
	else
		cerr << "[EE] Blad pobierania danych" << endl;

	// Identyfikacja krzywej
	cerr << "[RR] ";
	if (pre) fp << "# ";
	for (int i = 0; i < 25; i++)
	{
		cerr << (char)dane[i];
		if (pre) fp << (char)dane[i];
	}
	
	// Ilosc danych
	dane_size = 255*dane[26] + dane[27];	
  cerr << dane_size << endl;
	if (pre) fp << dane_size << "\n\n";

	// Zapisuje odczytane dane do tablicy
	int k = 0;
	for (int i = 27; i < 27+(4*dane_size)-36; i=i+4)
  {
		dane_dx[k] = (dane[i+0] * 256 + dane[i+1] - offset_x) * div_h * 0.01;
		dane_dy[k] = (dane[i+2] * 256 + dane[i+3] - offset_y) * div_v * 0.01;

		k++;
	}
	
	/* ====================================================================== */
	// Dane zapisujemy do pliku

	cerr << "[II] Zapisuje dane do pliku (\e[32;1m" << plik << "\e[0m)" << endl;
	for (int i = 0; i < k; i++)
		fp << dane_dx[i] << "\t" << dane_dy[i] << "\n";

	fp.close();

	/* ====================================================================== */
	// Konczymy zabawe
	
	cerr << "[II] Zwalniam interfejs GPIB" << endl;
	ibwrt( dev, "TEXt \"Pomiar zakonczony :-)\"", 28);
	ibwrt(dev, "DISPLAY NSTore", 14);
	ibclr(dev);
	ibloc(dev);
	ibonl(dev,0);
	return 0;
}

