//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ScrollSeg.h"

#include "Main.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm3 *Form3;
//---------------------------------------------------------------------------
__fastcall TForm3::TForm3(TComponent* Owner)
	: TForm(Owner)
{
	TStatusPanel * TSP ;
//	StatusBar1->Canvas
//	TSP->Text = L"Segundo = 0" ;

}
//---------------------------------------------------------------------------
void __fastcall TForm3::FormClose(TObject *Sender, TCloseAction &Action)
{
	Form1->ScrollSeg1->Checked = false ;
}
//---------------------------------------------------------------------------
void __fastcall TForm3::ScrollBar1Change(TObject *Sender)
{
	Form3->Caption =
		"Segundo visualizado: " +
			IntToStr(ScrollBar1->Position) +
		"   (Ultimo: " +
			IntToStr(ScrollBar1->Max) + ")" ;
}
//---------------------------------------------------------------------------

void __fastcall TForm3::ScrollBar1Scroll(TObject *Sender, TScrollCode ScrollCode,
		  int &ScrollPos)
{
	int Pos ;
	switch (ScrollCode) {
	case scLineUp    : Form1->AntSeg1Click(Form3) ; break ;
	case scLineDown  : Form1->SigSeg1Click(Form3) ; break ;
	case scPageUp    : Pos = ScrollPos - (ScrollBar1->Max/10) ;
					   if (Pos < 0) Pos = 0 ;
					   ScrollPos = Pos ;
					   Form1->irAlSegundo(ScrollPos) ; break ;
	case scPageDown  : Pos = ScrollPos + (ScrollBar1->Max/10) ;
					   if (Pos > ScrollBar1->Max)
						   Pos = ScrollBar1->Max ;
					   ScrollPos = Pos ;
					   Form1->irAlSegundo(ScrollPos) ; break ;
	case scPosition  : Form1->irAlSegundo(ScrollPos) ; 
					   break ;
	case scTrack     : break ;
	case scTop       : break ;
	case scBottom    : break ;
	case scEndScroll : break ;
	default: ;
	}
}
//---------------------------------------------------------------------------

