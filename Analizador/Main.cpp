﻿//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Main.h"

#include "Ir_a.h"                                                 /* Form2 */

#include "ScrollSeg.h"                         /* Form3, Form3->ScrollBar1 */

#include "sobre.h"                                     /* Form9 (Sobre...) */

#include "cmd.h"                                         /* mostrar_enlace */

#include "..\fda\usr\src\kernel\plotear.h"                /* opINT_00, ... */

extern AnsiString strIRQ [ 16 ] ; 
extern AnsiString strEXC [ 16 + 17 ] ;
extern AnsiString strSVC [ 16 + 17 + 10 ] ;
extern char digHex [ 17 ] ;                          /* "0123456789ABCFEF" */
extern AnsiString strHex [ 256 ] ;                  /* "00", "01", .. "FF" */	

//---------------------------------------------------------------------------

#pragma package(smart_init)
#pragma resource "*.dfm"

TForm1 *Form1;

//---------------------------------------------------------------------------

/* 
 * Inicializacion del formulario
 */

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
	int i, j ;
	
	FileName = L"" ;
	df_1 = -1 ;               /* para procesar las operaciones del fichero */
	df_off = -1 ;             /* para crear los índices off_seg y off_tick */
	df_size = -1 ;            /* para obtener el tamaño del fichero        */
	Contador = 0 ;                                    /* Contador de ticks */
	paso = false ;     

/*
 *  Los siguientes campos no se modifican tras la creacion del formulario
 */

	margenDerecho = 25 ;                                 /* margenDerecho  */
	shift = 10 ;                                         /* desplazamiento */
	ancho = shift ;                                      /* ancho columna  */
	alto = 13 ;                                          /* alto renglon   */

	xActual = Image1->Width - margenDerecho - shift ;         
	yLine = 4*alto + 2 ;                                       
	yFin  = Image1->Height - 2*alto ;
	yActual_0 = yLine + alto ;             
	
    Origen  = Rect(shift, 0,
			  Image1->Width - margenDerecho + shift, Image1->Height);
			  
	Destino = Rect(0, 0,
			  Image1->Width - margenDerecho, Image1->Height);
			  
	I = Image1 ;                                     /* solo para abreviar */
    C = Image1->Canvas ;                             /* solo para abreviar */
	
	C->Pen->Color = clRed;              /* lineas verticales de color rojo */
	C->CopyMode = cmSrcCopy ;	                   /* modo de copia normal */
	
	for ( i = 0 ; i < 16 ; i++ )                   /* "00", "01", ... "FF" */
		for ( j = 0 ; j < 16 ; j++ )
			strHex[16*i+j] =
				AnsiString(digHex[i]) +
				AnsiString(digHex[j]) ;

	borrar() ;
			
}
//---------------------------------------------------------------------------

/* off_seg [s] = desplazamiento del comienzo del segundo s en el fichero   */
/* off_tick[t] = desplazamiento del comienzo del tick    t en el fichero   */
/*                                                                         */
/* indice para localizar en Y:\log_e9.bin los segundos (opINT_00)          */
/* off_seg[s] = desplazamiento en Y:\log_e9.bin del opINT_00               */
/* correspondiente al comienzo del segundo, numero s < num_segs            */
/* Luego si ejecutamos:                                                    */
/*                                                                         */ 
/*     FileSeek(df_1, off_seg[s], SEEK_SET) ;                              */ 
/*     FileRead(df_1, &car, 1) ;                                           */  
/*                                                                         */
/* FileRead devuelve 1 y car = (char)opINT_00                              */
/* off_seg es actualizado con los nuevos ticks, de manera                  */
/* periodica mediante un timer (TimerIndice) con un periodo de             */
/* 100 milisegs a partir del fichero Y:\log_e9.bin.                        */
/*                                                                         */
/* off_tick[t] es lo mismo pero para los ticks                             */
/*                                                                         */ 
/* se cumplen: off_seg[s] = off_tick[60*s] y num_segs = num_ticks / 60     */
/*                                                                         */
/* por lo que podriamos prescindir de off_seg y de num_segs                */

//---------------------------------------------------------------------------
/*                                                                         */ 
/* actualizarIndice actualiza los indices periodicamente pos si crece el   */
/* tamaño del fichero. El fichero se supone abierto para esta tarea y con  */
/* el descriptor de fichero df_2                                           */ 
/*                                                                         */
/*   off_seg, num_segs, off_ticks, num_ticks                               */
/*                                                                         */

#define MAXBYTES 50000 

void __fastcall TForm1::actualizarIndice ( void )
{
	static int posActual = 0 ;                                 /* (STATIC) */

	unsigned op ;

    char car ;
	
/*  posActual es la posicion actual de procesamiento del fichero para la   */
/*  obtencion de los indices.                                              */
/*                                                                         */ 
/*  se utiliza un descriptor específico df_size para obtener el tamaño del */
/*  fichero sin trastocar la posicion actual en el descriptor df_2.        */

	int tamActual = FileSeek(df_size, 0, SEEK_END) ;  /* respecto al final */

    if ((tamActual - posActual) > MAXBYTES)
		tamActual = posActual + MAXBYTES ;  /* para salir antes de la r.c. */
                                          /* dejamos trabajo a TimerIndice */   
	while (posActual < tamActual)
	{
		FileRead(df_off, &car, 1) ;    /* leemos la operacion/traza actual */

		op = (unsigned)car ;

        switch (op) {
		case opIRQ_00 :                                   /* tick de reloj */
			off_tick[num_ticks++] = posActual ;  /* desplazam. de num_tick */
			if ((num_ticks % 60) == 1)             /* 60 ticks = 1 segundo */
			{
				off_seg[num_segs++] = posActual ;  /* desplaz. de num_segs */
//				Form3->ScrollBar1->Position = 0 ;
//				Form3->ScrollBar1->Min = 0 ;
				Form3->ScrollBar1->Max = num_segs-1 ;   /* 0 .. num_segs-1 */
				Form3->Caption =
					"Segundo visualizado: " +
						IntToStr(Form3->ScrollBar1->Position) +
					"   (Ultimo: " +
						IntToStr(Form3->ScrollBar1->Max) + ")" ;
			}
			posActual++ ;                                   /* opIRQ_XX 1B */
			break ;
		case opIRQ_01 : ;                          /* demas interrupciones */
		case opIRQ_02 : ;
		case opIRQ_03 : ;
		case opIRQ_04 : ;
		case opIRQ_05 : ;
		case opIRQ_06 : ;
		case opIRQ_07 : ;
		case opIRQ_08 : ;
		case opIRQ_09 : ;
		case opIRQ_10 : ;
		case opIRQ_11 : ;
		case opIRQ_12 : ;
		case opIRQ_13 : ;
		case opIRQ_14 : ;
		case opIRQ_15 : 
			posActual++ ;                                   /* opIRQ_XX 1B */
			break ;
		case opEXC_00 : ;                                   /* excepciones */
		case opEXC_01 : ;
		case opEXC_02 : ;
		case opEXC_03 : ;
		case opEXC_04 : ;
		case opEXC_05 : ;
		case opEXC_06 : ;
		case opEXC_07 : ;
		case opEXC_08 : ;
		case opEXC_09 : ;
		case opEXC_10 : ;
		case opEXC_11 : ;
		case opEXC_12 : ;
		case opEXC_13 : ;
		case opEXC_14 : ;
		case opEXC_15 : ;
		case opEXC_16 : 
			while (FileRead(df_off, &car, 1) == 0) { }      /* cod. error 1B */
			posActual = posActual + 2 ;           /* opEXC_XX + cod. error */ 
			break ;
		case opSVC_00 : ;                   /* llamadas al sistema (TRAPS) */
		case opSVC_01 : ;
		case opSVC_02 : ;
		case opSVC_03 : ;
		case opSVC_04 : ;
		case opSVC_05 : ;
		case opSVC_06 : ;
		case opSVC_07 : ;
		case opSVC_08 : ;
		case opSVC_09 : 
		    posActual++ ;                                   /* opSVC_XX 1B */
		    break ;
		case opMAPKBD : 
			while (FileRead(df_off, &car, 1) == 0) { } ;     /* scancode   */
			while (FileRead(df_off, &car, 1) == 0) { } ;     /* car. ascii */
			posActual = posActual + 3 ; /* opMAPKBD + scancode + car ascii */
			break ;
		case opIDE :                                         /* opcode  4B */
			for ( int i = 0 ; i < 16 ; i++ )                 /* sector  4B */
				while (FileRead(df_off, &car, 1) == 0) { } ; /* count   4B */	
            posActual = posActual + 17 ;				     /* do_dma  4B */
			break ;                                          /* total  16B */
		default : ;
		    Application->MessageBox (
				L"Tipo de traza incorrecto",
				L" Error en el fichero de trazas"
			) ;
            Application->ProcessMessages() ;
			Application->Terminate() ;
			return ;
		}
	}
}	

//---------------------------------------------------------------------------
void __fastcall TForm1::Abrir1Click(TObject *Sender)
{
	if (OpenDialog1->Execute())
	{
		FileName = OpenDialog1->FileName ;

/*      Se abre una segunda vez el fichero para calcular en paralelo el    */
/*      indice de desplazamientos de los segundos en el fichero            */

		if (((df_1    = FileOpen(FileName, fmOpenRead | fmShareDenyNone)) < 0) ||
		    ((df_off  = FileOpen(FileName, fmOpenRead | fmShareDenyNone)) < 0) ||
		    ((df_size = FileOpen(FileName, fmOpenRead | fmShareDenyNone)) < 0))
		{
			Application->MessageBox (
				L"El fichero no puede abrirse para lectura",
				L"Error de apertura"
			) ;
			if (df_1    > 0) FileClose(df_1   ) ;
			if (df_off  > 0) FileClose(df_off ) ;
			if (df_size > 0) FileClose(df_size) ;
			FileName = L"" ;
			df_1    = -1 ;
			df_off  = -1 ;
         	df_size = -1 ;  
			return ;
		}
		
		borrar() ;                            /* Borra el Canvas de Image1 */
		Abrir1->Enabled = false ;
		Cerrar1->Enabled = true ;
		Seguimiento1->Enabled = true ;
		Ver1->Enabled = true ;
		Caption = "Analizador  [" + FileName + "]" ;

		num_segs = 0 ;                               /* numero de segundos */
		Form3->ScrollBar1->Position = 0 ;
		Form3->ScrollBar1->Min = 0 ;
		Form3->ScrollBar1->Max = 0 ;
		
		actualizarIndice() ;   /* primeros MAXBYTES del fichero de momento */
		Inicio1Click(Form1) ;               /* mostramos el primer segundo */
#if 1
		ScrollSeg1->Checked = false ;    /* barra de desplazamiento oculta */ 
		Form3->Hide() ;                                    /* la ocultamos */  
#else
		ScrollSeg1->Checked = true ;    /* barra de desplazamiento visible */ 
		Form3->Show() ;                                    /* la mostramos */  
#endif
//		TimerIndice->Interval = 100 ;
		TimerIndice->Interval = 10 ;    /* periodo de actualizacion indice */ 
		TimerIndice->Enabled = true ;
	}
}

//---------------------------------------------------------------------------
void __fastcall TForm1::Cerrar1Click(TObject *Sender)
{
	FileName = L"" ;
	FileClose(df_1   ) ;
	FileClose(df_off ) ;
	FileClose(df_size) ;
	df_1    = -1 ;
	df_off  = -1 ;
	df_size = -1 ;
	Contador = 0 ;
	paso = false ;
	borrar() ;                                /* Borra el Canvas de Image1 */
	Abrir1->Enabled = true ;
	Cerrar1->Enabled = false ;
	Seguimiento1->Enabled = false ;
	Ver1->Enabled = false ;
	Ir_a1->Checked = false ;
	Form2->Hide() ;                       /* ocultamos formulario Ir a ... */
	ScrollSeg1->Checked = false ;
	Form3->Hide() ;                /* ocultamos la barra de desplazamiento */
	Caption = "Analizador" ;             /* quitamos el nombre del fichero */
	Panel1->Hide() ;          /* ocultamos el cursor de seleccion del tick */
	Panel2->Hide() ;
}

//---------------------------------------------------------------------------
void __fastcall TForm1::Salir1Click(TObject *Sender)
{
	Application->Terminate() ;
}

//---------------------------------------------------------------------------
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
	parse() ;
}

//---------------------------------------------------------------------------
void __fastcall TForm1::TimerIndiceTimer(TObject *Sender)
{
	actualizarIndice() ;
}

//---------------------------------------------------------------------------

//  Muestra el cronograma correspondiente a los dos primeros segundos

void __fastcall TForm1::Inicio1Click(TObject *Sender)
{
	int i ;

	borrar() ;                                /* Borra el Canvas de Image1 */
	FileSeek(df_1, 0, SEEK_SET) ;                     /* Esto es necesario */
	Contador = 0 ;                                    /* Contador de ticks */
    nOpsTick = 1 ;                                  
    yActual = yActual_0 + alto ;      /* primera operacion RECEIVE (clock) */                                
	paso = false ;
    Continuo1->Checked = false ; 
    parserEntry = 1 ;                      /* punto de entrada en parser() */
	
	C->Brush->Color = clBtnFace ;
	C->Pen->Color = clRed ;
	
	for ( i = 0 ; i < 62 ; i++ ) parse() ;           /* mostramos 62 ticks */

	Form2->Edit1->Text = IntToStr(0) ;                      /* Ir a tick 0 */ 
	Form3->ScrollBar1->Position = 0 ;   /* Barra de desplazamiento, tick 0 */

    numTickSel = -1 ;                          /* ningun tick seleccionado */
	Panel1->Hide() ;                        /* ocultamos el curso de ticks */
	Panel2->Hide() ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::irAlSegundo ( int seg )
{
	int i ;

	if (seg < 0)
		return ;
	else if (seg == 0)
		Inicio1Click(this) ;
	else if (seg == 1)
	{
    	borrar() ;                            /* Borra el Canvas de Image1 */
	    FileSeek(df_1, 0, SEEK_SET) ;                 /* Esto es necesario */
    	Contador = 0 ;                                /* Contador de ticks */
		nOpsTick = 1 ;
        yActual = yActual_0 + alto ;  /* primera operacion RECEIVE (clock) */                                
		paso = false ;
		Continuo1->Checked = false ;
        parserEntry = 1 ;                  /* punto de entrada en parser() */

		C->Brush->Color = clBtnFace ;
		C->Pen->Color = clRed ;

		for ( i = 0; i < 122 ; i++ ) parse() ;      /* mostramos 122 ticks */

    	Form2->Edit1->Text = IntToStr(1) ;                  /* Ir a tick 1 */ 
	    Form3->ScrollBar1->Position = 1 ;   /* Barra de desplazam., tick 1 */
	}
	else if (seg < num_segs)
	{
    	borrar() ;                            /* Borra el Canvas de Image1 */
		FileSeek(df_1, off_seg[seg-2]+1, SEEK_SET) ;
		Contador = (seg-2)*60 ;
		nOpsTick = 1 ;
		paso = false ;
		Continuo1->Checked = false ;
		parserEntry = 0 ;                  /* punto de entrada en parser() */

		C->Brush->Color = clBtnFace ;
		C->Pen->Color = clRed ;

		for ( i = 0; i < 181 ; i++ ) parse() ;      /* mostramos 181 ticks */

    	Form2->Edit1->Text = IntToStr(seg) ;              /* Ir a tick seg */ 
	    Form3->ScrollBar1->Position = seg ; /* Barra de desplaz., tick seg */
	}
//	else /* if (seg >= num_segs) */
//		return ;
	Panel1->Hide() ;                   /* ocultamos el cursor de los ticks */
	Panel2->Hide() ;
}

//---------------------------------------------------------------------------

//  Muestra el cronograma correspondiente a los dos ultimos segundos

void __fastcall TForm1::Final1Click(TObject *Sender)
{
	int i ;

	int seg_0, seg_1 ;

	seg_0 = num_segs-4 ;
	if (seg_0 < 0) seg_0 = 0 ;

   	borrar() ;                                /* Borra el Canvas de Image1 */

	FileSeek(df_1, off_seg[seg_0]+1, SEEK_SET) ;
	Contador = (seg_0)*60 ;
	nOpsTick = 1 ;
	paso = false ;
	Continuo1->Checked = false ;
	parserEntry = 0 ;                      /* punto de entrada en parser() */

	C->Brush->Color = clBtnFace ;
	C->Pen->Color = clRed ;

	for ( i = 0 ; i < 241 ; i++ ) parse() ;         /* mostramos 241 ticks */

	seg_1 = num_segs-1 ;
	if (seg_1 < 0) seg_1 = 0 ;
	Form2->Edit1->Text = IntToStr(seg_1) ;              /* Ir a tick seg_1 */
	Form3->ScrollBar1->Position = seg_1 ; /* Barra de desplaz., tick seg_1 */

	Panel1->Hide() ;                   /* ocultamos el cursor de los ticks */
	Panel2->Hide() ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SigOp1Click(TObject *Sender)
{
	int seg_1 ;

	paso = true ;
	parse() ;

	seg_1 = (Contador-1)/60-1 ;
	if (seg_1 < 0) seg_1 = 0 ;
	Form2->Edit1->Text = IntToStr(seg_1) ;              /* Ir a tick seg_1 */
	Form3->ScrollBar1->Position = seg_1 ; /* Barra de desplaz., tick seg_1 */
	
	Panel1->Hide() ;                   /* ocultamos el cursor de los ticks */
	Panel2->Hide() ;	
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SigTick1Click(TObject *Sender)
{
	int seg_1 ;

	parse() ;

	seg_1 = (Contador-1)/60-1 ;
	if (seg_1 < 0) seg_1 = 0 ;
	Form2->Edit1->Text = IntToStr(seg_1) ;              /* Ir a tick seg_1 */
	Form3->ScrollBar1->Position = seg_1 ; /* Barra de desplaz., tick seg_1 */

	Panel1->Hide() ;                   /* ocultamos el cursor de los ticks */
	Panel2->Hide() ;	
}
//---------------------------------------------------------------------------

/* Ir al siguiente segundo */

void __fastcall TForm1::SigSeg1Click(TObject *Sender)
{
	int seg_1 ;

	while ((parse() == 0) && ((Contador % 60) != 0)) ;
	parse() ;

	seg_1 = Contador/60 - 1 ;
	if (seg_1 < 0) seg_1 = 0 ;
	Form2->Edit1->Text = IntToStr(seg_1) ;              /* Ir a tick seg_1 */
	Form3->ScrollBar1->Position = seg_1 ; /* Barra de desplaz., tick seg_1 */

	Panel1->Hide() ;                   /* ocultamos el cursor de los ticks */
	Panel2->Hide() ;	
}
//---------------------------------------------------------------------------

/* Ir al segundo anterior */

void __fastcall TForm1::AntSeg1Click(TObject *Sender)
{
	int segActual = Contador/60 ;

	if (segActual <= 1)
		Inicio1Click(Sender) ;
	else
		irAlSegundo(segActual-2) ;
}
//---------------------------------------------------------------------------

/* Movimiento de la rueda del raton para scroll vertical */

void __fastcall TForm1::FormMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
		  TPoint &MousePos, bool &Handled)
{
	VertScrollBar->Position = VertScrollBar->Position - WheelDelta ;
}
//---------------------------------------------------------------------------

/* Arranque y parada de la visualizacion de los ticks */

void __fastcall TForm1::StartStop1Click(TObject *Sender)
{
	 if (Parado1->Checked) 
	 {                                                        /* activamos */
		 Timer1->Enabled  = true ;
		 Timer1->Interval = 1000/60 ;
		 Parado1         ->Checked = false ;
		 Frecuencia60Hz1 ->Checked = true ;
    	 Panel1->Hide() ;              /* ocultamos el cursor de los ticks */
	     Panel2->Hide() ;	

	 }
	 else 
	 {                                                     /* desactivamos */
		 Timer1->Enabled = false ;
		 Parado1         ->Checked = true  ;
		 Frecuencia15Hz1 ->Checked = false ;
		 Frecuencia30Hz1 ->Checked = false ;
		 Frecuencia45Hz1 ->Checked = false ;
		 Frecuencia60Hz1 ->Checked = false ;
		 Frecuencia90Hz1 ->Checked = false ;
		 Frecuencia120Hz1->Checked = false ;
		 Frecuencia240Hz1->Checked = false ;
		 Frecuencia480Hz1->Checked = false ;
		 Frecuencia960Hz1->Checked = false ;
		 Continuo1       ->Checked = false ;
	 }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Parado1Click(TObject *Sender)
{
	 Timer1->Enabled = false ;
	 Parado1         ->Checked = true  ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia15Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/15 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = true  ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia30Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/30 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = true  ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia45Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/45 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = true  ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
}

//---------------------------------------------------------------------------
void __fastcall TForm1::Frecuencia60Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/60 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = true  ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia90Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/90 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = true  ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia120Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/120 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = true  ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia240Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/240 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = true  ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia480Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/420 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = true  ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Frecuencia960Hz1Click(TObject *Sender)
{
	 Timer1->Enabled = true ;
	 Timer1->Interval = 1000/960 ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = true  ;
	 Continuo1       ->Checked = false ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Continuo1Click(TObject *Sender)
{
	 Timer1->Enabled = false ;
	 Parado1         ->Checked = false ;
	 Frecuencia15Hz1 ->Checked = false ;
	 Frecuencia30Hz1 ->Checked = false ;
	 Frecuencia45Hz1 ->Checked = false ;
	 Frecuencia60Hz1 ->Checked = false ;
	 Frecuencia90Hz1 ->Checked = false ;
	 Frecuencia120Hz1->Checked = false ;
	 Frecuencia240Hz1->Checked = false ;
	 Frecuencia480Hz1->Checked = false ;
	 Frecuencia960Hz1->Checked = false ;
	 Continuo1       ->Checked = true  ;
	 parse() ;
}
//---------------------------------------------------------------------------
 
void __fastcall TForm1::Ir_a1Click(TObject *Sender)
{
	if (Ir_a1->Checked) 
	{
		Ir_a1->Checked = false ;
		Form2->Hide() ;
	}
	else
	{
		Ir_a1->Checked = true ;
		Form2->Show() ;
	}
}
//---------------------------------------------------------------------------

/* Mostrar/Ocultar barra de desplazamiento de ticks */

void __fastcall TForm1::ScrollSeg1Click(TObject *Sender)
{
	if (ScrollSeg1->Checked) 
	{
		ScrollSeg1->Checked = false ;
		Form3->Hide() ;
	}
	else
	{
		ScrollSeg1->Checked = true ;
		Form3->Show() ;
	}
}
//---------------------------------------------------------------------------

/* caracteres a escribir para opIRQ_XX, opEXC_XX y opSVC_XX */

AnsiString strIRQ [ 16 ] =
{
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"A", "B", "C", "D", "E", "F"
} ;

AnsiString strEXC [ 16 + 17 ] =
{
	"#", "#", "#", "#", "#", "#", "#", "#", "#", "#",
	"#", "#", "#", "#", "#", "#",
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"A", "B", "C", "D", "E", "F", "G"
} ;

AnsiString strSVC [ 16 + 17 + 10 ] =
{
	"#", "#", "#", "#", "#", "#", "#", "#", "#", "#",
	"#", "#", "#", "#", "#", "#",
	"#", "#", "#", "#", "#", "#", "#", "#", "#", "#",
	"#", "#", "#", "#", "#", "#",
	"#",
//	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
//  .., SEND, RECEIVE, SENDREC, NOTIFY, ... , ECHO 
	"0", "S", "R", "C", "N", "5", "6", "7", "E", "9"
} ;

/* tablas para escribir el scancode XX provemiente del teclado */

char digHex [ 17 ] = "0123456789ABCDEF" ;

AnsiString strHex [ 256 ] ;                         /* "00", "01", .. "FF" */

//---------------------------------------------------------------------------

#define setFont( _Name, _Height, _Color ) \
	C->Font->Name = _Name ;               \
	C->Font->Height = _Height ;           \
	C->Font->Color = _Color ;             

/* Borra la Imagen */

void __fastcall TForm1::borrar ( void )
{
//	C->Brush->Color = clWhite ;
//	C->Pen->Color = clWhite ;
	C->Brush->Color = clBtnFace ;
	C->Pen->Color = clBtnFace ;
	C->Rectangle(
		0, 0, Form1->Image1->Width, Form1->Image1->Height
	) ;
}

/* Pinta el numero de segundos en el cronograma */

void __fastcall TForm1::pintarSegundos ( void )
{
	if ((Contador % 60) == 0)
	{
//      pintamos el numero de segundos transcurridos

		int segundos = Contador/60 ;
		int xSegs ;
		int ySegs ;

		ySegs = yLine - 20 ;
		if      (segundos <   10) xSegs = xActual - 1*5 + 2 ;
		else if (segundos <  100) xSegs = xActual - 2*5 + 2 ;
		else if (segundos < 1000) xSegs = xActual - 3*5 + 0 ;
		else                      xSegs = xActual - 4*5 - 2 ;

		setFont("Tahona", 12, clRed) ;
		C->Font->Style = TFontStyles() << fsBold ;

		C->TextOut(xSegs, ySegs, Format("%d", segundos)) ;

		if (num_segs > 0)
			Form3->ScrollBar1->Position =
				((segundos*Form3->ScrollBar1->Max)/num_segs) ;
		else
			Form3->ScrollBar1->Position = 0 ;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::pintarLinea ( void ) 
{
//  pintamos la linea de separacion vertical

//  nos ponemos a la altura del comienzo de la línea

	C->MoveTo(xActual, yLine);

//  establecemos el tipo de linea (psSolid/psDot)

	if ((Contador % 60) == 0)            /* comienza un segundo */
		C->Pen->Style = psSolid ;
	else if (((Contador % 60) % 6) == 0)
	{                                    /* comienza una decima */
		C->Pen->Style = psSolid ;
		C->LineTo(xActual, yLine+8);
		C->Pen->Style = psDot ;
	}
	else                             /* comienza un tick normal */
		C->Pen->Style = psDot ;

	C->LineTo(xActual, yFin);	
}	
//---------------------------------------------------------------------------

void __fastcall TForm1::pintarIntReloj ( void ) 
{
//  pintamos la interrupción de reloj (IRQ = 0)
	C->MoveTo(xActual + 2, yActual);
    setFont("Tahona", 12, clRed) ;
	C->Font->Style = TFontStyles() << fsBold ;
//	C->TextOut(C->PenPos.x, C->PenPos.y, strIRQ[0]) ;	
	C->TextOut(C->PenPos.x, C->PenPos.y, "0") ;	            /* por rapidez */
}	
//---------------------------------------------------------------------------

void __fastcall TForm1::pintarTecla ( void ) 
{
	unsigned char scode ;                                    /* scancode   */
	char ch ;                                                /* ascii char */  

	if (nOpsTick < 61) 
	{	
        setFont("Tahona", 12, clPurple) ;
		C->Font->Style = TFontStyles() << fsBold ;
		C->MoveTo(xActual + 2, yActual);
	   	C->TextOut(C->PenPos.x, C->PenPos.y, "M") ;           /* de MAPKBD */
	    yActual += alto ;
    }
				
	while (FileRead(df_1, &scode, 1) == 0) { }            /* clock_counter */
	while (FileRead(df_1, &ch   , 1) == 0) { }            /* clock_counter */
	C->Font->Name = "Tahona" ;
	if (scode & 0x80)                                        /* liberacion */
	{
//		C->Font->Color = clBlack ;
		C->Font->Color = clGray ;
		C->Font->Style = TFontStyles() ;
	}
	else                                                      /* pulsacion */
	{
		C->Font->Color = clRed ;
		C->Font->Style = TFontStyles() << fsBold ;
	}
	C->Font->Height = 12 ;
	C->TextOut(
//		xActual - 2*5 - 10,
		xActual-3,
		yLine-20-2*alto,
		strHex[scode]
	) ;
//	if ((' ' < ch) && (ch < DEL)) {
//	if ((' ' < ch) && (ch < 0x7F)) {
//	if (isprint(ch))
	if (isgraph(ch))
	{
		C->TextOut(
//	    	xActual - 2*5 - 10,
			xActual + 2,
			yLine-20-1*alto,
			AnsiString(ch)
		) ;
	}	
}	
//---------------------------------------------------------------------------

void __fastcall TForm1::pintarNumOpsTick ( void )
{
// indicamos el numero de operaciones total en el tick */

	String str ;
	int i ;

	setFont("Tahona", 12, clBlack) ;
	C->Font->Style = TFontStyles() << fsBold ;
			
	str = Format("%5d", nOpsTick) ;

	for ( i = 1 ; i <= 5 ; i++ )
	{
		C->MoveTo(xActual + 2, yActual);
		C->TextOut(C->PenPos.x, C->PenPos.y, str[i]) ;
		yActual += alto ;
	}
}	
//---------------------------------------------------------------------------

int __fastcall TForm1::parse ( void )
{

//  continuamos la ejecucion por donde habia quedado

	switch (parserEntry) {
	case 1 : goto parserEntry_1 ;
	case 2 : goto parserEntry_2 ;
	default : ; /* goto parserEntry_0 */
	}

	do
	{
		
parserEntry_0: ;

//      se ha leído un byte op = opIRQ_00 (interrupcion del timer)
//      comienzo de un nuevo tick

		pintarSegundos() ;
		pintarLinea() ;
		Contador++ ;                          /* nuevo tick */
		yActual = yActual_0 ;
        pintarIntReloj() ;
		yActual += alto ;                      /* avanzamos */
		nOpsTick = 1 ;                    /* ops en el tick */

//      bucle de lectura y pintado de las ops en ese tick   */

		do
		{

parserEntry_1: ;

//          se intenta leer la siguiente operacion

			if (FileRead(df_1, &car, 1) == 0)
//			while (FileRead(df_1, &car, 1) == 0)
			{
//				Application->ProcessMessages();
				parserEntry = 1 ; /* para la siguiente activacion */
				return parserEntry ;
			}
			parserEntry = 0 ;

			op = (unsigned)car ;

			if (op == opIRQ_00) { }
			else if (op <= opSVC_09)
			{
				nOpsTick++ ;                            /* nueva operacion */

				if (nOpsTick < 61)
				{
					C->MoveTo(xActual + 2, yActual);

					if (op <= opIRQ_15)                    /* interrupcion */
					{
						setFont("Tahona", 12, clRed) ;
						C->Font->Style = TFontStyles() << fsBold ;
						C->TextOut(C->PenPos.x, C->PenPos.y, strIRQ[op]) ;
					}
					else if (op <= opEXC_16)                  /* excepcion */
					{

// En el caso de una excepcion hay que leer el codigo de error

parserEntry_2: ;
						while (FileRead(df_1, &car, 1) == 0) /* trap_errno */
						{
//       				    Application->ProcessMessages();
// Realmente no es necesario retornar ya que esta
// asegurado que va a aparecer en car el codigo de error
//						    parserEntry = 2 ;
//          				return parserEntry ;
						}
						parserEntry = 0 ;

						setFont("Tahona", 12, clBlack) ;
						C->Font->Style = TFontStyles() ;
						C->TextOut(C->PenPos.x, C->PenPos.y, strEXC[op]) ;
					}
					else /* if (op <= opSVC_09) */ /* trap */
					{
						setFont("Tahona", 12, clBlue) ;
						C->Font->Style = TFontStyles() ;
						C->TextOut(C->PenPos.x, C->PenPos.y, strSVC[op]) ;
					}

					yActual += alto ; /* avanzamos */

				}
				else if (nOpsTick == 61) {
					setFont("Tahona", 12, clPurple) ;
					C->Font->Style = TFontStyles() << fsBold ;
					C->MoveTo(xActual + 2, yActual);
					C->TextOut(C->PenPos.x, C->PenPos.y, "V") ;
					yActual += alto ;
				}

				if ((!Timer1->Enabled) && paso)
				{
					paso = false ;
					parserEntry = 1 ; /* para la siguiente activacion */
					return parserEntry ;
				}
			}
#if 1
			else if (op == opMAPKBD)
			{
				pintarTecla() ;
			}
#if 0
			else if (op == opINTHND) {
				int irq ;
				while (FileRead(df_1, &car, 1) == 0) { } /* clock_counter */
				while (FileRead(df_1, &car, 1) == 0) { } /* clock_counter */

				while (FileRead(df_1, &car, 1) == 0) { } /* irq */
				irq = ((unsigned)car) & 0x0F ;
				while (FileRead(df_1, &car, 1) == 0) { } /* hook_id    */
				if (irq == 1) /* teclado */
				{
					while (FileRead(df_1, &car, 1) == 0) { }
					setFont("Tahona", 12, clRed) ;
					C->Font->Style = TFontStyles() << fsBold ;
					C->TextOut(
						xActual - 2*5 - 10,
						yLine-20-2*alto,
//						Format("3%d", ((int)car))
						strHex[(unsigned)(car & 0xFF)]
//                      "AB"
					) ;
				}
			}
			else if (op == opEXCHND) 
			{
				while (FileRead(df_1, &car, 1) == 0) { } /* clock_counter */
				while (FileRead(df_1, &car, 1) == 0) { } /* clock_counter */

				while (FileRead(df_1, &car, 1) == 0) { }

				setFont("Tahona", 12, clRed) ;
				C->Font->Style = TFontStyles() << fsBold ;
				C->TextOut(C->PenPos.x, C->PenPos.y, "X") ;
			}
#endif

#if 1
			else if (op == opIDE) 
			{
				for ( int j = 0 ; j < 16 ; j++ ) {
					while (FileRead(df_1, &car, 1) == 0) { } ;
				}				
			}	
#else 
			else if (op == opIDE) 
			{
				for ( int j = 0 ; j < 16 ; j++ ) {
					while (FileRead(df_1, &car, 1) == 0) { } ;
				}
				setFont("Tahona", 12, clPurple) ;
				C->Font->Style = TFontStyles() << fsBold ;
				C->TextOut(C->PenPos.x, C->PenPos.y, "H") ;
			}
#endif
			
#endif

		}
		while (op != opIRQ_00) ;

		if (nOpsTick >= 61)
			pintarNumOpsTick() ;

		C->CopyRect(Destino, C, Origen);

//		Application->ProcessMessages();
		if (Continuo1->Checked) {
			Application->ProcessMessages();
		}
	}
	while (Continuo1->Checked) ;

//  Sleep(100) ;

	return 0 ;

}
//---------------------------------------------------------------------------

#if 0

int __fastcall TForm1::posTick ( int num ) /* en Image1 */
{
	return num*num ;
}

int __fastcall TForm1::posTickSel ( void ) /* en Image1 */
{
	return posTick(numTickSel) ;
}

int __fastcall TForm1::numTickEn ( int X ) /* X relativa a Form1 */
{
	int k ;
	int xAlign ; /* coordenada x alineada a tick */

/* Las coordenadas (X, Y) son relativas al comienzo de la    */
/* imagen contenida en Form1.                                */
/* Otras coordenadas (Panel1->Left, Panel1->Top) son         */
/* relativas al comienzo del rectangulo visible, por lo que  */
/* hay que tener en cuenta las posiciones de las barras de   */
/* scroll horizontal y vertical.                             */

/*  xAlign = xActual - k*shift */ /* para algun k */

	k = (xActual - (HorzScrollBar->Position + X) + shift)/shift ;

	xAlign = xActual - k*shift ;

//	if (FileSeek(df_1, 0, SEEK_SET) < FileSeek(df_1, 0, SEEK_END))
	if (Contador/60 < num_segs-1)  /* depurar */
		return (Contador - k) ;
	else
		return (Contador - 1 - k) ;
}

bool __fastcall TForm1::tickVisible ( int num )
{
	return true ;
}

bool __fastcall TForm1::tickSelVisible ( void )
{
	return tickVisible(numTickSel) ;
}

void __fastcall TForm1::mostrarTick ( int num )
{
	return ;
}

void __fastcall TForm1::mostrarTickSel ( void )
{
	return mostrarTick(numTickSel) ;
}

void __fastcall TForm1::ocultarTickSel ( void )
{
    return ;
}

#endif

//---------------------------------------------------------------------------

void __fastcall TForm1::FormMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  int X, int Y)
{
	int xAlign ; /* coordenada x alineada a tick */
	int k ; /* ticks de diferencia Contador y seleecionado */
	int seg ; /* segundo seleccionado */
	int tick ;  /* tick selecionado */
	int numOps ;

	if (Abrir1->Enabled) return ;

/* Las coordenadas (X, Y) son relativas al comienzo de la    */
/* imagen contenida en Form1.                                */
/* Las coordenadas (Panel1->Left, Panel1->Top) son relativas */
/* al comienzo del rectangulo visible, por lo que hay que    */
/* tener en cuenta las posiciones de las barras de scroll    */
/* horizontal y vertical.                                    */

/*  xAlign = xActual - k*shift */ /* para algun k */

	k = (xActual - (HorzScrollBar->Position + X) + shift)/shift ;

	xAlign = xActual - k*shift ;

//	if (FileSeek(df_1, 0, SEEK_SET) < FileSeek(df_1, 0, SEEK_END))
	if (Contador/60 < num_segs-1)  /* depurar */
		numTickSel = Contador - k ;
	else
		numTickSel = Contador - 1 - k ;
	seg = numTickSel / 60 ;
	tick = numTickSel % 60 ;

/*  comprobamos que no estamos fuera de los ticks */

	if ((seg < 0) ||
		((seg == 0) && (tick < -1)) ||
		(HorzScrollBar->Position + X > xActual))
		return ;

	Panel1->Left = xAlign - HorzScrollBar->Position ;
	Panel1->Width = ancho + 3 ;
	Panel1->Top = Form1->yLine - VertScrollBar->Position - 3 ;
	Panel1->Height = Form1->yLine + 58*alto ;

	Panel2->Left = xAlign - HorzScrollBar->Position + Panel1->Width - 2 ;
//  Panel2->Width = 225 ;
	Panel2->Top = Form1->yLine - VertScrollBar->Position - 3 ;
	Panel2->Height = Form1->yLine + 58*alto ;

//	StatusBar1->Panels->Items[0]->Width = 158 ;
//	StatusBar1->Panels->Items[1]->Width = 30 ;
//	StatusBar1->Panels->Items[2]->Width = 30 ;
//  Panel2->Width = 225 ;

/*  actualizamos la barra de estado */

	StatusBar1->Panels->Items[0]->Text =
		String(L"  Seg  ") + IntToStr(seg) +
		String(L"  Tick  ") + IntToStr(tick) +
		String(L"  Ops  ") + IntToStr(100) ;

	Panel1->Show() ;
	Panel2->Hide() ;

/*  rellenamos RichEdit1 con las operaciones del tick */

	numOps = analizarTick(numTickSel) ;

	StatusBar1->Panels->Items[0]->Text =
		String(L"  Seg  ") + IntToStr(seg) +
		String(L"  Tick  ") + IntToStr(tick) +
		String(L"  Ops  ") + IntToStr(numOps) ;

}
//---------------------------------------------------------------------------

int __fastcall TForm1::analizarTick ( int numTickSel )
{
	int df_3 = FileOpen(FileName, fmOpenRead | fmShareDenyNone) ;

	int tamActual = FileSeek(df_size, 0, SEEK_END) ;

	int op, i, j ;

	unsigned sc ; /* scancode */

	char car ;

	AnsiString str ;

	FileSeek(df_3, off_tick[numTickSel] + 1, SEEK_SET) ;

	RichEdit1->Lines->Clear() ;

	i = 0 ;

	RichEdit1->Lines->Add(
//		String(L"") + 
		Format("%2d:", i++) +
		String(L" INT IRQ =  0 (TIMER) ")
	) ;

	while (FileSeek(df_3, 0, SEEK_CUR) < tamActual)
	{

		if ((i % 100) == 99)
			Application->ProcessMessages() ;

		FileRead(df_3, &car, 1) ;

		op = (unsigned)car ;

		if (op == opIRQ_00)
			break ;
		else if (op <= opIRQ_15)
		{
			if (op == opIRQ_01)
				RichEdit1->Lines->Add(
//					String(L"") +
					Format("%2d:", i++) +
					Format(" INT IRQ =  1 (TECLADO) ")
				) ;
			else if (op != opIRQ_14)
				RichEdit1->Lines->Add(
//					String(L"") +
					Format("%2d:", i++) +
					Format(" INT IRQ = %2d ", op)
				) ;
			else if (op == opIRQ_14)
				RichEdit1->Lines->Add(
//					String(L"") + 
					Format("%2d:", i++) +
					String(L" INT IRQ = 14 (HD) ")
				) ;
		}
		else if (op <= opEXC_16)
		{
			while (FileRead(df_3, &car, 1) == 0) { }
			i++ ;
		}
		else if (op <= opSVC_09)
		{
			str = 
//			    String(L"") + 
				Format("%2d: ", i++) ;
			switch (op) {
			case opSVC_00 : break ;
			case opSVC_01 : str = str + "SEND        " ; break ;
			case opSVC_02 : str = str + "RECEIVE     " ; break ;
			case opSVC_03 : str = str + "SENDREC     " ; break ;
			case opSVC_04 : str = str + "NOTIFY      " ; break ;
			case opSVC_05 : str = str + "IPC_REQUEST " ; break ;
			case opSVC_06 : str = str + "IPC_REPLY   " ; break ;
			case opSVC_07 : str = str + "IPC_NOTIFY  " ; break ;
			case opSVC_08 : str = str + "ECHO        " ; break ;
			case opSVC_09 : str = str + "IPC_RECEIVE " ; break ;
			default: ;
			}
			RichEdit1->Lines->Add(str) ;
		}
		else if (op == opMAPKBD)
		{
#if 1
			while (FileRead(df_3, &car, 1) == 0) { } ;
			sc = 0x000000FF & (unsigned)car ;
			while (FileRead(df_3, &car, 1) == 0) { } ;
#else
			FileRead(df_3, &car, 1) ;
			sc = 0x000000FF & (unsigned)car ;
			FileRead(df_3, &car, 1) ;
#endif
			i = i + 2 ;
			RichEdit1->Lines->Add(
//				String(L"") +
				Format("%2d:", i++) +
				Format(" MAPKBD SC = %s ASCII = \'%s\'",
					ARRAYOFCONST((strHex[sc], car))
				)
			) ;
		}
		else if (op == opIDE)
		{
			struct trazaIDE {
				unsigned int opcode ;                                          /* PP */
				unsigned int sector ;                                          /* PP */
				unsigned int count ;                                           /* PP */
				unsigned int do_dma ;
			} trazaIDE ;
			char * ptr = (char *)&trazaIDE ;											 /* PP */
			for ( j = 0 ; j < 16 ; j++ ) {
				while (FileRead(df_3, &ptr[j], 1) == 0) { } ;
			}
			i = i + 16 ;
			RichEdit1->Lines->Add(
//				String(L"") +
				Format("%2d:", i++) +
				Format(" IDE op = %d S = %d C = %d dma = %d",
					ARRAYOFCONST((
						trazaIDE.opcode,
						trazaIDE.sector,
						trazaIDE.count,
						trazaIDE.do_dma
					))
				)
			) ;
		}
		else { }
	}
	return (i + 1) ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Panel1MouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  int X, int Y)
{
	if (Panel2->Visible)
		Panel2->Hide() ;
	else
		Panel2->Show() ;
}
//---------------------------------------------------------------------------

bool SBResizing = false ;
int SBX ;

void __fastcall TForm1::StatusBar1MouseDown(TObject *Sender, TMouseButton Button,
		  TShiftState Shift, int X, int Y)
{
	if (!SBResizing)
	{
		if (X > (StatusBar1->Left + StatusBar1->Width) -
					 StatusBar1->Panels->Items[2]->Width)
		{
			SBResizing = true ;
			SBX = X ;
			StatusBar1->Panels->Items[2]->Text = "X   " ;
		}
		else if (X > (StatusBar1->Left +
					  StatusBar1->Panels->Items[0]->Width))
		{
			FormMouseDown(
				Form1, Button, Shift,
				Panel2->Left + ancho/2, yLine
			) ;
            Panel2->Show() ;
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::StatusBar1MouseMove(TObject *Sender, TShiftState Shift, int X,
		  int Y)
{
	if (!SBResizing) return ;
	if (Panel2->Width + (X - SBX) >= 225)
	{
		Panel2->Width = Panel2->Width + (X - SBX) ;
		SBX = X ;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::StatusBar1MouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  int X, int Y)
{
	if (SBResizing)
	{
		SBResizing = false ;
		StatusBar1->Panels->Items[2]->Text = "R   " ;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Sobre1Click(TObject *Sender)
{
    Form9->Show() ;
}
//---------------------------------------------------------------------------


void __fastcall TForm1::Ayuda2Click(TObject *Sender)
{
	mostrar_enlace("https://github.com/Stichting-MINIX-Research-Foundation/minix/tree/R3.1.2") ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::usrsrcmpx386s1Click(TObject *Sender)
{
	mostrar_enlace("https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/R3.1.2/kernel/mpx386.s") ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::usrsrcmpx386s2Click(TObject *Sender)
{
	mostrar_enlace("https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/R3.1.2/kernel/i8259.c") ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::usrsrckernelexceptionc1Click(TObject *Sender)
{
	mostrar_enlace("https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/R3.1.2/kernel/exception.c") ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::usrsrcdriversttykeyboardc1Click(TObject *Sender)
{
	mostrar_enlace("https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/R3.1.2/drivers/tty/keyboard.c") ;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::usrsrcdriversttykeyboardc2Click(TObject *Sender)
{
	mostrar_enlace("https://github.com/Stichting-MINIX-Research-Foundation/minix/blob/R3.1.2/drivers/at_wini/at_wini.c") ;
}
//---------------------------------------------------------------------------

/* Para ocultar el cursor de los ticks al presionar la tecla Esc */

void __fastcall TForm1::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)

{
	if (Key == vkEscape) {                 /* ocultamos el cursor de ticks */
		Panel1->Hide() ;
        Panel2->Hide() ;
	}
}
//---------------------------------------------------------------------------
