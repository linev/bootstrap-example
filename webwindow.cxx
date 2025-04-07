/// \file
/// \ingroup tutorial_webgui
/// \ingroup webwidgets
/// Minimal server/client code for working with RWebWindow class.
///
/// File webwindow.cxx shows how RWebWindow can be created and used
/// In webwindow.html simple client code is provided.
///
/// \macro_code
///
/// \author Sergey Linev

#include <ROOT/RWebWindow.hxx>

#include "TCanvas.h"
#include "TWebCanvas.h"
#include "TTimer.h"
#include "TFile.h"
#include "TH2.h"

std::shared_ptr<ROOT::RWebWindow> window;

TCanvas *canvas = nullptr;
TWebCanvas *web_canv = nullptr;
TH2I *hist = nullptr;

int counter = 0;

void SetPrivateCanvasFields(TCanvas *canv, bool on_init = true)
{
   Long_t offset = TCanvas::Class()->GetDataMemberOffset("fCanvasID");
   if (offset > 0) {
      Int_t *id = (Int_t *)((char *) canv + offset);
      if (*id == canv->GetCanvasID()) *id = on_init ? 111222333 : -1;
   } else {
      printf("ERROR: Cannot modify TCanvas::fCanvasID data member\n");
   }

   offset = TCanvas::Class()->GetDataMemberOffset("fPixmapID");
   if (offset > 0) {
      Int_t *id = (Int_t *)((char *) canv + offset);
      if (*id == canv->GetPixmapID()) *id = on_init ? 332211 : -1;
   } else {
      printf("ERROR: Cannot modify TCanvas::fPixmapID data member\n");
   }

   offset = TCanvas::Class()->GetDataMemberOffset("fMother");
   if (offset > 0) {
      TPad **moth = (TPad **)((char *) canv + offset);
      if (*moth == canv->GetMother()) *moth = on_init ? canv : nullptr;
   } else {
      printf("ERROR: Cannot set TCanvas::fMother data member\n");
   }

   offset = TCanvas::Class()->GetDataMemberOffset("fCw");
   if (offset > 0) {
      UInt_t *cw = (UInt_t *)((char *) canv + offset);
      if (*cw == canv->GetWw()) *cw = on_init ? 800 : 0;
   } else {
      printf("ERROR: Cannot set TCanvas::fCw data member\n");
   }

   offset = TCanvas::Class()->GetDataMemberOffset("fCh");
   if (offset > 0) {
      UInt_t *ch = (UInt_t *)((char *) canv + offset);
      if (*ch == canv->GetWh()) *ch = on_init ? 600 : 0;
   } else {
      printf("ERROR: Cannot set TCanvas::fCw data member\n");
   }
}

void LoadTCanvas(const char *file_name, const char* canvas_name)
{

   auto f = TFile::Open(file_name);
   canvas = (TCanvas *) f->Get(canvas_name);

   //canvas = new TCanvas(kFALSE);
   // canvas->SetName(canvas_name);
   // canvas->SetTitle(canvas_name);
   canvas->ResetBit(TCanvas::kShowEditor);
   canvas->ResetBit(TCanvas::kShowToolBar);
   canvas->SetBit(TCanvas::kMenuBar, kTRUE);
   canvas->SetCanvas(canvas);
   canvas->SetBatch(kTRUE); // mark canvas as batch
   canvas->SetEditable(kTRUE); // ensure fPrimitives are created

   Bool_t readonly = kFALSE;

   // create implementation
   web_canv = new TWebCanvas(canvas, "title", 0, 0, 800, 600, readonly);

   // use async mode to prevent blocking
   // web_canv->SetAsyncMode(kTRUE);

   // assign implementation
   canvas->SetCanvasImp(web_canv);
   SetPrivateCanvasFields(canvas, true);
   canvas->cd();

   {
      R__LOCKGUARD(gROOTMutex);
      auto l1 = gROOT->GetListOfCleanups();
      if (!l1->FindObject(canvas))
         l1->Add(canvas);
      auto l2 = gROOT->GetListOfCanvases();
      if (!l2->FindObject(canvas))
         l2->Add(canvas);
   }

   // ensure creation of web window
   web_canv->ShowWebWindow("embed");
}

void ProcessData(unsigned connid, const std::string &arg)
{
   printf("Get msg %s \n", arg.c_str());

   counter++;

   if (arg == "get_text") {
      // send arbitrary text message
      window->Send(connid, TString::Format("Message%d", counter).Data());
   } else if (arg == "get_binary") {
      // send float array as binary
      float arr[10];
      for (int n = 0; n < 10; ++n)
         arr[n] = counter;
      window->SendBinary(connid, arr, sizeof(arr));
   } else if (arg == "halt") {
      // terminate ROOT
      window->TerminateROOT();
   } else if (arg.compare(0, 8, "channel:") == 0) {
      int chid = std::stoi(arg.substr(8));
      printf("Get channel request %d\n", chid);

      ROOT::RWebWindow::ShowWindow(web_canv->GetWebWindow(), { window, connid, chid });
   }
}

int fill_place = 2;

void update_canvas()
{
   hist->Fill(fill_place*2, fill_place, 1000);
   fill_place = (fill_place + 7) % 60;
   canvas->Modified();
   canvas->Update();
}

void webwindow()
{
   // create window
   window = ROOT::RWebWindow::Create();

   LoadTCanvas("file_h_adc_chan_0.root", "c_h_adc_chan_0");

   //hist = new TH1I("hpx", "Test histogram", 40, -5, 5);
   //hist->FillRandom("gaus", 10000);
   //canvas->Add(hist);

   hist = (TH2I*) canvas->GetListOfPrimitives()->FindObject("h_adc_chan_0");

   auto timer = new TTimer("update_canvas()", 2000, kFALSE);
   timer->TurnOn();

   // configure default html page
   // either HTML code can be specified or just name of file after 'file:' prefix
   std::string fdir = __FILE__;
   auto pos = fdir.find("webwindow.cxx");
   if (pos > 0)
      fdir.resize(pos);
   else
      fdir = "./";
   window->SetDefaultPage("file:" + fdir + "dist/index.html");

   // this is call-back, invoked when message received from client
   window->SetDataCallBack(ProcessData);

   window->SetGeometry(1200, 800); // configure predefined geometry

   window->Show();
}
